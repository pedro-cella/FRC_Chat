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
    char username[20];  // Adicionando o nome de usuário
} Participant;


// Declare uma lista global de participantes
Participant participants[MAX_PARTICIPANTS];
int numParticipants = 0;

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

    printf("Nova conexão: cliente %d\n", newfd);
    fflush(stdout);  // Limpar o buffer de saída antes de ler a entrada do usuário

    // Enviar uma mensagem para o cliente solicitar o nome de usuário
    char msg[] = "Digite seu nome de usuário: ";
    send(newfd, msg, sizeof(msg), 0);

        // Solicitar o nome de usuário e atribuir o ID único ao participante
    Participant newParticipant;
    newParticipant.id = newfd;

    // Receber o nome de usuário do cliente
    memset(&buf, 0, sizeof(buf));
    nbytes = recv(newfd, buf, sizeof(buf), 0);
    if (nbytes <= 0) {
        // Lidar com erro de leitura
        printf("Erro ao receber o nome de usuário do cliente %d\n", newfd);
        close(newfd);
        FD_CLR(newfd, &master);
        return;
    }

    // Copiar o nome de usuário para a estrutura do participante
    strncpy(newParticipant.username, buf, sizeof(newParticipant.username) - 1);
    newParticipant.username[sizeof(newParticipant.username) - 1] = '\0';

    // Adicionar o novo participante à lista de participantes
    participants[numParticipants] = newParticipant;
    numParticipants++;

    printf("Usuário '%s' conectado com sucesso\n", newParticipant.username);
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

// Função responsável por listar os usuários conectados no servidor
void list_users() {
    if (numParticipants == 0) {
        printf("Nenhum usuário está conectado.\n");
    } else {
        printf("Usuários conectados:\n");
        for (int i = 0; i < numParticipants; i++) {
            printf("%d. [ID: %d] Usuário: %s", i+1, participants[i].id, participants[i].username);
        }
    }
}

void join_room(int roomIndex, int participantFd) {
    if (roomIndex >= 0 && roomIndex < numRooms) {
        ChatRoom *room = &chatRooms[roomIndex];
        if (room->numParticipants >= MAX_PARTICIPANTS) {
            printf("A sala '%s' atingiu o limite máximo de participantes\n", room->name);
            return;
        }

        if (numParticipants == 0) {
            printf("Nenhum usuário conectado no momento\n");
            return;
        }

        list_users();  // Mostrar os usuários conectados
        printf("Digite o ID do usuário que deseja adicionar: ");
        int userId;
        scanf("%d", &userId);

        // Verificar se o usuário com o ID fornecido está conectado
        int userIndex = -1;
        for (int i = 0; i < numParticipants; i++) {
            if (participants[i].id == userId) {
                userIndex = i;
                break;
            }
        }

        if (userIndex != -1) {
            // Verificar se o usuário já está na sala
            for (int i = 0; i < room->numParticipants; i++) {
                if (room->participants[i].id == userId) {
                    printf("Usuário já está na sala\n");
                    return;
                }
            }

            // Adicionar o participante à sala
            room->participants[room->numParticipants] = participants[userIndex];
            room->numParticipants++;
            printf("Usuário '%s' adicionado à sala '%s'\n", participants[userIndex].username, room->name);
        } else {
            printf("Usuário não encontrado\n");
        }
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
            printf("%d. %s (ID: %d)\n", i + 1, room->participants[i].username, room->participants[i].id);
        }
    } else {
        printf("Sala não encontrada\n");
    }
}

void show_help() {
    printf("Comandos disponíveis:\n");
    printf("/create <nome_sala> - Cria uma nova sala\n");
    printf("/join <índice_sala> - Entra em uma sala existente\n");
    printf("/list - Lista as salas disponíveis\n");
    printf("/users <índice_sala> - Lista os participantes de uma sala\n");
    printf("/help - Mostra esta mensagem de ajuda\n");
    printf("\n");
}

int main(int argc, char *argv[]) {
    setup_socket(argc, argv);
    int isFirstConnection = 1;

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
                        } else if (strncmp(input, "/users", 6) == 0) {
                            list_users();
                        } else if (strncmp(input, "/help", 5) == 0) {
                            show_help();
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
