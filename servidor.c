#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "./constants.h"

// Variáveis associadas a conexão
fd_set master, read_fds, write_fds;
struct sockaddr_in myaddr, remoteaddr;
int fdmax, sd, newfd, i, j, nbytes, yes = 1;
socklen_t addrlen;
char buf[MAX_MSG_SIZE];

typedef struct {
  char name[MAX_ROOM_NAME_LENGTH];
  int clients[MAX_CLIENTS_PER_ROOM];
  int numClients;
  int roomID;  // Identificador da sala
} ChatRoom;

ChatRoom chatRooms[MAX_ROOMS];
int numRooms = 0;
int idGenerator = 0;

int server_response(int socket, char* msg) {

    if (socket == 0){
        printf("%s", msg);
    } else {
        send(socket, msg, strlen(msg), 0);
    }
    return 1;
}

void create_room(const char* roomName) {
  // Verificar se o número máximo de salas já foi alcançado
  if (numRooms >= MAX_ROOMS) {
    printf("Limite máximo de salas alcançado.\n");
    return;
  }

  // Verificar se o nome da sala é muito longo
  if (strlen(roomName) >= MAX_ROOM_NAME_LENGTH) {
    printf("O nome da sala é muito longo.\n");
    return;
  }

  // Verificar se o nome da sala já está em uso
  for (int i = 0; i < numRooms; i++) {
    if (strcmp(chatRooms[i].name, roomName) == 0) {
      printf("Já existe uma sala com esse nome.\n");
      return;
    }
  }

  // Criar a nova sala
  ChatRoom newRoom;  // Declara uma variável do tipo ChatRoom para representar a
                     // nova sala

  strcpy(newRoom.name,
         roomName);  // Copia o nome fornecido para o campo 'name' da nova sala

  // Inicializa o número de clientes da nova sala como
  newRoom.numClients = 0;  // 0, já que não há clientes conectados ainda

  newRoom.roomID = idGenerator++;

  // Adiciona a nova sala ao array de salas
  chatRooms[numRooms] = newRoom;  // 'chatRooms' na posição 'numRooms'

  numRooms++;  // Incrementa o contador de salas para refletir a adição da nova
               // sala

  printf("Sala '%s' criada com sucesso.\n", roomName);
}

void delete_room(const char* roomName) {
  int roomIndex = -1;

  // Procura pelo índice da sala com o nome fornecido
  for (int i = 0; i < numRooms; i++) {
    if (strcmp(chatRooms[i].name, roomName) == 0) {
      roomIndex = i;
      break;
    }
  }

  // Verifica se a sala foi encontrada
  if (roomIndex == -1) {
    printf("Sala '%s' não encontrada.\n", roomName);
    return;
  }

  // Remove a sala do array de salas
  for (int i = roomIndex; i < numRooms - 1; i++) {
    chatRooms[i] = chatRooms[i + 1];
  }

  numRooms--;

  printf("Sala '%s' removida com sucesso.\n", roomName);
}

void list_rooms(int client_fd) {
  if (numRooms == 0) {
    server_response(client_fd, "Não há salas disponíveis\n");
  } else {
    server_response(client_fd, "Salas disponíveis:\n");
    for (int i = 0; i < numRooms; i++) {
      char roomInfo[MAX_MSG_SIZE];
      snprintf(roomInfo, sizeof(roomInfo), "%d. %s (%d/%d participantes)\n",
               i + 1, chatRooms[i].name, chatRooms[i].numClients,
               MAX_CLIENTS_PER_ROOM);
      server_response(client_fd, roomInfo);
    }
  }
}

// void envia_msg(int sender_fd) {
//     for (j = 0; j <= fdmax; j++) {
//         if (FD_ISSET(j, &master)) {
//             if ((j != sender_fd) && (j != sd)) {
//                 send(j, buf, nbytes, 0);
//             }
//         }
//     }
// }

void envia_msg(int sender_fd, int roomID) {
  for (int i = 0; i < chatRooms[roomID].numClients; i++) {
    int client_fd = chatRooms[roomID].clients[i];
    if (client_fd != sender_fd) {
      send(client_fd, buf, nbytes, 0);
    }
  }
}

void setup_socket(char* ip, char* port) {
  FD_ZERO(&master);
  FD_ZERO(&read_fds);

  sd = socket(AF_INET, SOCK_STREAM, 0);
  setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

  myaddr.sin_family = AF_INET;
  myaddr.sin_addr.s_addr = inet_addr(ip);
  myaddr.sin_port = htons(atoi(port));

  memset(&(myaddr.sin_zero), '\0', 8);
  addrlen = sizeof(remoteaddr);
  bind(sd, (struct sockaddr*)&myaddr, sizeof(myaddr));

  listen(sd, 10);
  FD_SET(sd, &master);
  FD_SET(STDIN, &master);
  fdmax = sd;
}

void list_connections() {
  printf("Conexões ativas:\n");

  // Imprimir o servidor
  printf("Servidor (socket %d)\n", sd);

  // Imprimir os clientes
  for (int i = 0; i <= fdmax; i++) {
    if (FD_ISSET(i, &master) && i != sd) {
      if (i == 0) {
        printf("Servidor STDIN (socket %d)\n", i);  // Remove o Cliente 0
      } else {
        printf("Cliente %d\n", i);
      }
    }
  }
}

void send_room_list(int client_fd) {
  char roomList[MAX_ROOMS * MAX_ROOM_NAME_LENGTH];
  memset(roomList, 0, sizeof(roomList));

  // Concatena as informações das salas na string roomList, separadas por quebra
  // de linha
  for (int i = 0; i < numRooms; i++) {
    char roomInfo[512];  // Tamanho aumentado para acomodar a string formatada
    int roomInfoLength = snprintf(
        roomInfo, sizeof(roomInfo), "%d. %s (%d/%d participantes)\n", i + 1,
        chatRooms[i].name, chatRooms[i].numClients, MAX_CLIENTS_PER_ROOM);
    if (roomInfoLength < 0 || roomInfoLength >= sizeof(roomInfo)) {
      printf("Erro ao formatar informações da sala %d\n", i + 1);
      continue;
    }
    strcat(roomList, roomInfo);
  }

  send(client_fd, roomList, strlen(roomList), 0);
}

int get_room_id(int client_fd) {
  for (int i = 0; i < numRooms; i++) {
    for (int j = 0; j < chatRooms[i].numClients; j++) {
      if (chatRooms[i].clients[j] == client_fd) {
        return chatRooms[i].roomID;
      }
    }
  }
  return -1;  // Cliente não está em nenhuma sala
}

int is_online(int i) {
  // Processa o input do cliente
  int response = server_response(i, "");
  printf("R: %d\n", response);
  if (response == -1) {
    // Conexão fechada pelo cliente
    close(i);
    FD_CLR(i, &read_fds);
    return 0;
  }
  return 1;
}

int execute_command(int i, char *input) {
  if (strncmp(input, "/create", 7) == 0) {
    char roomName[MAX_ROOM_NAME_LENGTH];
    sscanf(input, "/create %[^\n]", roomName);
    create_room(roomName);
  } else if (strncmp(input, "/list", 5) == 0) {
    list_rooms(i);
  } else if (strncmp(input, "/delete", 7) == 0) {
    char roomName[MAX_ROOM_NAME_LENGTH];
    sscanf(input, "/delete %[^\n]", roomName);
    delete_room(roomName);
  } else if (strncmp(input, "/clear", 6) == 0) {
    system(CLEAR_COMMAND);
  } else {
    printf("SOCKET %d: %s\n", i, input);
  }
}

void handle_command(int i) {
  char input[MAX_MSG_SIZE];
  size_t bytes_read;
  int msg_size = 0;

  if (i == STDIN) {
    // Processa input do STDIN
    fgets(input, sizeof(input), stdin);
    if (strncmp(input, "/connections", 12) == 0) {
      list_connections();
    }
  } else {
    while ((bytes_read =
                read(i, input + msg_size, sizeof(input) - msg_size - 1)) > 0) {
      msg_size += bytes_read;
      if (msg_size > MAX_MSG_SIZE - 1 || input[msg_size - 1] == '\n') {
        break;
      }
    }
  }

  execute_command(i, input);
}

int handle_new_connection() {
  newfd = accept(sd, (struct sockaddr*)&remoteaddr, &addrlen);
  FD_SET(newfd, &master);
  if (newfd > fdmax) fdmax = newfd;
  return 1;
}

int main(int argc, char* argv[]) {
  if (argc < 3) {
    printf("Digite IP e Porta para este servidor\n");
    exit(1);
  }

  setup_socket(argv[1], argv[2]);

  for (;;) {
    read_fds = master;
    select(fdmax + 1, &read_fds, NULL, NULL, NULL);

    for (i = 0; i <= fdmax; i++) {
      if (FD_ISSET(i, &read_fds)) {
        if (i == sd) {
          handle_new_connection();
        } else {
          handle_command(i);
        }
      }
    }
  }

  return 0;
}
