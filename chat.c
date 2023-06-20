#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <unistd.h>

#define STDIN 0
#define MAX_ROOMS 10
#define MAX_ROOM_NAME 50
#define MAX_PARTICIPANTS 10

typedef struct {
    int id;
} Participant;

typedef struct {
    char name[MAX_ROOM_NAME];
    Participant participants[MAX_PARTICIPANTS];
    int numParticipants;
} ChatRoom;


fd_set master, read_fds, write_fds;
struct sockaddr_in myaddr, remoteaddr;
int fdmax, sd, newfd, i, j, nbytes, yes = 1;
socklen_t addrlen;
char buf[256];
ChatRoom chatRooms[MAX_ROOMS];
int numRooms = 0;

void send_msg(int sender_fd) {
    for (j = 0; j <= fdmax; j++) {
        if (FD_ISSET(j, &master)) {
            if ((j != sender_fd) && (j != sd)) {
                send(j, buf, nbytes, 0);
            }
        }
    }
}

void setup_socket(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Digite IP e Porta para este servidor\n");
        exit(1);
    }

    FD_ZERO(&master);
    FD_ZERO(&read_fds);

    sd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    myaddr.sin_family = AF_INET;
    myaddr.sin_addr.s_addr = inet_addr(argv[1]);
    myaddr.sin_port = htons(atoi(argv[2]));

    memset(&(myaddr.sin_zero), '\0', 8);
    addrlen = sizeof(remoteaddr);

    bind(sd, (struct sockaddr *)&myaddr, sizeof(myaddr));
    listen(sd, 10);

    FD_SET(sd, &master);
    FD_SET(STDIN, &master);

    fdmax = sd;
}

void handle_new_connection() {
    newfd = accept(sd, (struct sockaddr *)&remoteaddr, &addrlen);
    FD_SET(newfd, &master);
    if (newfd > fdmax)
        fdmax = newfd;
}

void receive_and_send_messages() {
    memset(&buf, 0, sizeof(buf));
    nbytes = recv(i, buf, sizeof(buf), 0);
    send_msg(i);
}

void create_room(char *roomName) {
    if (numRooms >= MAX_ROOMS) {
        printf("Limite máximo de salas atingido\n");
        return;
    }

    for (i = 0; i < numRooms; i++) {
        if (strcmp(chatRooms[i].name, roomName) == 0) {
            printf("Já existe uma sala com esse nome\n");
            return;
        }
    }

    strcpy(chatRooms[numRooms].name, roomName);
    chatRooms[numRooms].numParticipants = 0;
    numRooms++;

    printf("Sala '%s' criada com sucesso\n", roomName);
}

void join_room(int roomIndex, int participantFd) {
    if (roomIndex >= 0 && roomIndex < numRooms) {
        ChatRoom *room = &chatRooms[roomIndex];
        if (room->numParticipants >= MAX_PARTICIPANTS) {
            printf("A sala '%s' atingiu o limite máximo de participantes\n", room->name);
            return;
        }

        int participantId;
        printf("Digite um identificador único: ");
        scanf("%d", &participantId);

        // Verifica se o identificador já está em uso
        for (i = 0; i < room->numParticipants; i++) {
            if (room->participants[i].id == participantId) {
                printf("Identificador já em uso. Tente novamente.\n");
                return;
            }
        }

        room->participants[room->numParticipants].id = participantId;
        room->numParticipants++;

        printf("Você entrou na sala '%s'\n", room->name);
    } else {
        printf("Sala não encontrada\n");
    }
}



void list_rooms() {
    if (numRooms == 0) {
        printf("Não há salas disponíveis\n");
    } else {
        printf("Salas disponíveis:\n");
        for (i = 0; i < numRooms; i++) {
            printf("%d. %s (%d/%d participantes)\n", i + 1, chatRooms[i].name,
                   chatRooms[i].numParticipants, MAX_PARTICIPANTS);
        }
    }
}

void list_participants(int roomIndex) {
    if (roomIndex >= 0 && roomIndex < numRooms) {
        ChatRoom *room = &chatRooms[roomIndex];
        printf("Participantes da sala '%s':\n", room->name);
        for (i = 0; i < room->numParticipants; i++) {
            printf("%d. Participante %d\n", i + 1, room->participants[i].id);
        }
    } else {
        printf("Sala não encontrada\n");
    }
}

int main(int argc, char *argv[]) {
    setup_socket(argc, argv);

    for (;;) {
        read_fds = master;
        select(fdmax + 1, &read_fds, NULL, NULL, NULL);

        for (i = 0; i <= fdmax; i++) {
            if (FD_ISSET(i, &read_fds)) {
                if (i == sd) {
                    handle_new_connection();
                } else {
                    if (i == STDIN) {
                        // Process input from STDIN
                        char input[256];
                        fgets(input, sizeof(input), stdin);

                        // Check if it's a command to create a room
                        if (strncmp(input, "/create", 7) == 0) {
                            char roomName[MAX_ROOM_NAME];
                            sscanf(input, "/create %[^\n]", roomName);
                            create_room(roomName);
                        } else if (strncmp(input, "/join", 5) == 0) {
                            int roomIndex;
                            sscanf(input, "/join %d", &roomIndex);
                            join_room(roomIndex - 1, i);
                        } else if (strncmp(input, "/list", 5) == 0) {
                            list_rooms();
                        } else if (strncmp(input, "/list_participants", 18) == 0) {
                            int roomIndex;
                            sscanf(input, "/list_participants %d", &roomIndex);
                            list_participants(roomIndex - 1);
                        } else {
                            printf("Comando inválido\n");
                        }
                    } else {
                        receive_and_send_messages();
                    }
                }
            }
        }
    }

    return 0;
}
