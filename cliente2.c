#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>


int createSocket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erro ao criar o socket");
        return -1;
    }
    return sockfd;
}

int connectToServer(int sockfd, const char* serverIP, int serverPort) {
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(serverPort);
    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
        perror("Erro ao converter o endereço IP");
        return -1;
    }
    if (connect(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        perror("Erro ao conectar ao servidor");
        return -1;
    }
    return 0;
}

int sendData(int sockfd, const char* data, int length) {
    int bytesSent = send(sockfd, data, length, 0);
    if (bytesSent < 0) {
        perror("Erro ao enviar dados para o servidor");
        return -1;
    }
    return bytesSent;
}

int receiveData(int sockfd, char* buffer, int bufferSize) {
    int bytesRead = recv(sockfd, buffer, bufferSize - 1, 0);
    if (bytesRead < 0) {
        perror("Erro ao receber dados do servidor");
        return -1;
    }
    buffer[bytesRead] = '\0';  // Adiciona um terminador de string no final dos dados recebidos
    return bytesRead;
}

void closeSocket(int sockfd) {
    close(sockfd);
}

void printRoomList(char* roomList) {
    printf("Salas disponíveis:\n");
    char* roomInfo = strtok(roomList, "\n");
    int count = 1;
    while (roomInfo != NULL) {
        printf("%d. %s\n", count, roomInfo);
        roomInfo = strtok(NULL, "\n");
        count++;
    }
}


int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Digite IP e Porta para se conectar ao servidor\n");
        return 1;
    }

    const char* serverIP = argv[1];
    int serverPort = atoi(argv[2]);

    int sockfd = createSocket();
    if (sockfd < 0) {
        return 1;
    }

    if (connectToServer(sockfd, serverIP, serverPort) < 0) {
        closeSocket(sockfd);
        return 1;
    }
    
    // Recebe a lista de salas do servidor
    char roomList[256];
    int bytesRead = receiveData(sockfd, roomList, sizeof(roomList));
    if (bytesRead < 0) {
        closeSocket(sockfd);
        return 1;
    }
    printRoomList(roomList);

    char input[256];
    while (1) {
        // Lê a entrada do usuário
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';  // Remove o caractere de nova linha do final

        // Verifica se o usuário quer sair
        if (strcmp(input, "/quit") == 0) {
            break;
        }

        // Envia os dados para o servidor
        if (sendData(sockfd, input, strlen(input)) < 0) {
            closeSocket(sockfd);
            return 1;
        }

        // Recebe a resposta do servidor
        char buffer[256];
        int bytesRead = receiveData(sockfd, buffer, sizeof(buffer));
        if (bytesRead < 0) {
            closeSocket(sockfd);
            return 1;
        }
        printf("Resposta do servidor: %s\n", buffer);
    }

    closeSocket(sockfd);

    return 0;
}
