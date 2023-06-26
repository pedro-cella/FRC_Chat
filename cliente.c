#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define BUFFER_SIZE 256

void error(const char *msg) {
    perror(msg);
    exit(1);
}

int createSocket() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("Erro ao abrir o socket");
    return sockfd;
}

void connectToServer(int sockfd, const char *hostname, int portno) {
    struct sockaddr_in serv_addr;
    struct hostent *server;

    server = gethostbyname(hostname);
    if (server == NULL)
        error("Erro, host não encontrado");

    bzero((char *)&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
        error("Erro ao conectar");
}

void sendMessage(int sockfd, const char *message) {
    int n = write(sockfd, message, strlen(message));
    if (n < 0)
        error("Erro ao escrever no socket");
}

void receiveMessage(int sockfd, char *buffer) {
    bzero(buffer, BUFFER_SIZE);
    int n = read(sockfd, buffer, BUFFER_SIZE - 1);
    if (n < 0)
        error("Erro ao ler do socket");
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Digite IP e Porta para se conectar ao servidor\n");
        exit(1);
    }

    int sockfd, portno;
    char buffer[BUFFER_SIZE];

    portno = atoi(argv[2]);
    sockfd = createSocket();
    connectToServer(sockfd, argv[1], portno);

    printf("Conectado ao servidor.\n");

    receiveMessage(sockfd, buffer);
    printf("%s\n", buffer);

    while (1) {
        bzero(buffer, BUFFER_SIZE);
        fgets(buffer, BUFFER_SIZE - 1, stdin);

        sendMessage(sockfd, buffer);

        if (strcmp(buffer, "/quit\n") == 0) {
            break;
        }

        receiveMessage(sockfd, buffer);
        printf("%s\n", buffer);

        if (strcmp(buffer, "Você foi desconectado.\n") == 0) {
            // Encerra a conexão se o servidor enviar a mensagem de desconexão
            break;
        }
    }

    close(sockfd);
    return 0;
}
