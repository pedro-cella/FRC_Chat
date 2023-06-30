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
  int admUser; // Trocar para um vetor de adm, fazer func de add adm.
} ChatRoom;

ChatRoom chatRooms[MAX_ROOMS];
int numRooms = 0;
int idGenerator = 0; // Verificar importancia dessa var.

int server_response(int socket, char* msg) {
  // printf("Len: %d; Str: %s\n", strlen(msg), msg);

    if (socket == 0){
        printf("%s", msg);
    } else {
        send(socket, msg, strlen(msg), 0);
    }
    return 1;
}

char *copystring(char *restrict dest, char const *restrict source, size_t elements) {
    char *d;
    for (d = dest; d + 1 < dest + elements; d++, source++)
        *d = *source;
    *d = '\0';

    return d;

}

// void copystring(char* dest, char* src, size_t elements) {
//   size_t length = strlen(src);
//   memcpy(dest, src, length);
// }


int compare(char *str1, char *str2) {

  printf("Comparing s1: %s; s2: %s.\n", str1, str2);

  while (*str1 && *str1 == *str2) {
    str1++;
    str2++;
  }
  printf("*str1 - *str2 = %d\n", *str1 - *str2);
  return *str1 - *str2;

}

// Apresentou problemas quando dois clientes de sockets diferentes tentaram entrar na mesma sala
int search_client_in_room(int socket){

  for (int i = 0; i < numRooms; i++){
    for (int j = 0; i < chatRooms[i].numClients; j++){
      if (chatRooms[i].clients[j] == socket){
        return i;
      }
    }
  }

  return -1;
}

int find_client_room(int socket) {
  for (int i = 0; i < numRooms; i++) {
    for (int j = 0; j < chatRooms[i].numClients; j++) {
      if (chatRooms[i].clients[j] == socket) {
        return i; // Retorna o número da sala em que o cliente está
      }
    }
  }
  
  return -1; // Cliente não está em nenhuma sala
}


//TODO - Ao criar a sala ele engole o último caractere do nome
void create_room(int socket, char* roomName) {
  // Verificar se o número máximo de salas já foi alcançado
  if (numRooms >= MAX_ROOMS) {
    server_response(socket, "Limite máximo de salas alcançado.\n");
    return;
  }

  // Verificar se o nome da sala é muito longo
  if (strlen(roomName) >= MAX_ROOM_NAME_LENGTH) {
    server_response(socket, "O nome da sala é muito longo.\n");
    return;
  }

  // Verificar se o nome da sala já está em uso
  for (int i = 0; i < numRooms; i++) {
    if (strncmp(chatRooms[i].name, roomName, strlen(chatRooms[i].name)) == 0) {
      server_response(socket, "Já existe uma sala com esse nome.\n");
      return;
    }
  }

  // Criar a nova sala
  ChatRoom newRoom;  // Declara uma variável do tipo ChatRoom para representar a
                     // nova sala
  // strcpy(newRoom->name, roomName);  // Copia o nome fornecido para o campo 'name' da nova sala
  // Inicializa o número de clientes da nova sala como
  copystring(newRoom.name, roomName, strlen(roomName));
  newRoom.numClients = 0;  // 0, já que não há clientes conectados ainda
  newRoom.roomID = idGenerator++;
  newRoom.admUser = i;//! Trocar para vetor.
  // Adiciona a nova sala ao array de salas
  chatRooms[numRooms] = newRoom;  // 'chatRooms' na posição 'numRooms'
  numRooms++;  // Incrementa o contador de salas para refletir a adição da nova
               // sala

  char msg[MAX_MSG_SIZE];
  snprintf(msg, sizeof(msg), "Sala '%s' criada com sucesso.\n", newRoom.name);
  server_response(socket, msg);
}

int search_room_name(char * room_name){
  copystring(room_name, room_name, strlen(room_name));

  printf("Searching for: %s\n", room_name);

  for (int i = 0; i < numRooms; i++) {
    if (compare(chatRooms[i].name, room_name) == 0) {
      return i;
    }
  }
  return -1;
}

int delete_room(int socket, char* roomName) {

  int roomIndex = search_room_name(roomName);
  if (roomIndex == -1){
    // Sala não existe.
      printf("Room index: %d\n", roomIndex);
      server_response(socket, "Sala selecionada não existe.\n");
      return 0;
  }

  // Remove a sala do array de salas
  for (int i = roomIndex; i < numRooms - 1; i++) {
    chatRooms[i] = chatRooms[i + 1];
  }
  numRooms--;

  char msg[MAX_MSG_SIZE];
  snprintf(msg, sizeof(msg) + 30, "Sala '%s' removida com sucesso.\n", roomName);
  server_response(socket, msg);
}

int join_room(int socket, char *room_name){

  printf("join_room: %s\n", room_name);

  int room_index = search_room_name(room_name);
  if (room_index == -1){
    // Sala não existe.
      printf("Room index: %d\n", room_index);
      server_response(socket, "Sala selecionada não existe.\n");
      return 0;
  }


  //? Possibilidade de troca de sala.
  int result_search = find_client_room(socket);
  
  if (result_search >= 0){
    server_response(socket, "Usuario está conectado em outra sala.\n");
    return 0;
  }


  ChatRoom *room = &chatRooms[room_index];

  if(room->numClients >= MAX_CLIENTS_PER_ROOM){
    // Sala cheia sem espaço.
    server_response(socket, "Sala cheia.\n");
    return 0;
  }

  // Cliente adicionado.
  room->clients[room->numClients] = socket;
  room->numClients++;
  char msg[MAX_MSG_SIZE];
  snprintf(msg, sizeof(msg), "Entrou na sala %s.\n", room->name);
  server_response(socket, msg);
  return 1;

}

int leave_room(int socket) {
  int roomIndex = find_client_room(socket);
  
  if (roomIndex == -1) {
    // Cliente não está em nenhuma sala
    server_response(socket, "Você não está em nenhuma sala.\n");
    return 0;
  }
  
  ChatRoom* room = &chatRooms[roomIndex];
  
  for (int i = 0; i < room->numClients; i++) {
    if (room->clients[i] == socket) {
      // Remove o cliente da sala
      for (int j = i; j < room->numClients - 1; j++) {
        room->clients[j] = room->clients[j + 1];
      }
      room->numClients--;
      break;
    }
  }
  
  // Envia mensagem de confirmação ao cliente
  server_response(socket, "Você saiu da sala.\n");
  
  return 1;
}

// int change_room(int socket, char* new_room_name) {
//   int currentRoomIndex = find_client_room(socket);
//   int newRoomIndex = search_room_name(new_room_name);
//   char roomName[MAX_ROOM_NAME_LENGTH];
//   copystring(roomName, new_room_name, strlen(new_room_name));

//   printf("ROOMNAME: %s\n", roomName);

//   if (currentRoomIndex == -1) {
//     // Cliente não está em nenhuma sala
//     server_response(socket, "Você não está em nenhuma sala.\n");
//     return 0;
//   }

//   if (newRoomIndex == -1) {
//     // A sala não existe
//     server_response(socket, "Essa sala não existe.\n");
//     return 0;
//   }

//   leave_room(socket);
  
//   if (newRoomIndex != -1) {
//     join_room(socket, new_room_name);
//   } else {
//     join_room(socket, roomName);
//   }
  
//   return 1;
// }


int change_room(int socket, char* new_room_name) {
  int current_room_index = find_client_room(socket);
  if (current_room_index == -1) {
    // Cliente não está em nenhuma sala
    server_response(socket, "Você não está em nenhuma sala.\n");
    return 0;
  }
  
  leave_room(socket);
  
  int new_room_index = search_room_name(new_room_name);
  if (new_room_index != -1) {
    join_room(socket, new_room_name);
  } else {
    join_room(socket, chatRooms[current_room_index].name);
  }
  
  return 1;
}


void list_rooms(int client_fd) {
  if (numRooms == 0) {
    server_response(client_fd, "Não há salas disponíveis\n");
  } else {
    server_response(client_fd, "Salas disponíveis:\n");
    for (int i = 0; i < numRooms; i++) {
      char roomInfo[MAX_MSG_SIZE];
      snprintf(roomInfo, sizeof(roomInfo), "%d. %s (%d/%d participantes)\n", i + 1, chatRooms[i].name, chatRooms[i].numClients, MAX_CLIENTS_PER_ROOM);
      server_response(client_fd, roomInfo);
    }
  }
}
//! Quase funcionando. 
void envia_msg(int sender_fd, int room_index) {
  
  for (int i = 0; i < chatRooms[room_index].numClients; i++) {
    int client_fd = chatRooms[room_index].clients[i];
    if (client_fd != sender_fd) {
      send(client_fd, buf, nbytes, 0);
    }
  }
}

void send_message(int sender_fd, int room_index, char* message) {
  
  for (int i = 0; i < chatRooms[room_index].numClients; i++) {
    int client_fd = chatRooms[room_index].clients[i];
    if (client_fd != sender_fd) {
      send(client_fd, message, strlen(message), 0);
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

// void send_room_list(int client_fd) {
//   char roomList[MAX_ROOMS * MAX_ROOM_NAME_LENGTH];
//   memset(roomList, 0, sizeof(roomList));

//   // Concatena as informações das salas na string roomList, separadas por quebra
//   // de linha
//   for (int i = 0; i < numRooms; i++) {
//     char roomInfo[512];  // Tamanho aumentado para acomodar a string formatada
//     int roomInfoLength = snprintf(
//         roomInfo, sizeof(roomInfo), "%d. %s (%d/%d participantes)\n", i + 1,
//         chatRooms[i].name, chatRooms[i].numClients, MAX_CLIENTS_PER_ROOM);
//     if (roomInfoLength < 0 || roomInfoLength >= sizeof(roomInfo)) {
//       printf("Erro ao formatar informações da sala %d\n", i + 1);
//       continue;
//     }
//     strcat(roomList, roomInfo);
//   }

//   send(client_fd, roomList, strlen(roomList), 0);
// }

// int get_room_id(int client_fd) {
//   for (int i = 0; i < numRooms; i++) {
//     for (int j = 0; j < chatRooms[i].numClients; j++) {
//       if (chatRooms[i].clients[j] == client_fd) {
//         return chatRooms[i].roomID;
//       }
//     }
//   }
//   return -1;  // Cliente não está em nenhuma sala
// }

// int is_online(int i) {
//   // Processa o input do cliente
//   int response = server_response(i, "");
//   printf("R: %d\n", response);
//   if (response == -1) {
//     // Conexão fechada pelo cliente
//     close(i);
//     FD_CLR(i, &read_fds);
//     return 0;
//   }
//   return 1;
// }

int execute_command(int i, char *input) {
  if (strncmp(input, "/create", 7) == 0) {
    char roomName[MAX_ROOM_NAME_LENGTH];
    sscanf(input, "/create %[^\n]", roomName);
    create_room(i, roomName);
    printf("%s\n", chatRooms[numRooms-1].name);
  } else if (strncmp(input, "/list", 5) == 0) {
    list_rooms(i);
  } else if (strncmp(input, "/join", 5) == 0){
    char roomName[MAX_ROOM_NAME_LENGTH];
    sscanf(input, "/join %[^\n]", roomName);
    join_room(i, roomName);
  } else if (strncmp(input, "/delete", 7) == 0) {
    char roomName[MAX_ROOM_NAME_LENGTH];
    sscanf(input, "/delete %[^\n]", roomName);
    delete_room(i, roomName);
  } else if (strncmp(input, "/change", 7) == 0) {
    char roomName[MAX_ROOM_NAME_LENGTH];
    sscanf(input, "/change %[^\n]", roomName);
    change_room(i, roomName);
  } else if (strncmp(input, "/clear", 6) == 0) {
    system(CLEAR_COMMAND);
  } else if(strncmp(input, "/leave", 6) == 0){
    leave_room(i);
  }else {
    // Teste
    int roomIndex = find_client_room(i);
    if(roomIndex != -1){
      char message[MAX_MSG_SIZE];
      snprintf(message, sizeof(message), "%s\n", input);
      send_message(i, roomIndex, message);
    }else{
      printf("SOCKET %d: %s\n", i, input);
    }
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
      while ((bytes_read = read(i, input + msg_size, sizeof(input) - msg_size - 1)) > 0) {
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
