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

fd_set master, read_fds, write_fds;
struct sockaddr_in myaddr, remoteaddr;
int fdmax, sd, newfd, i, j, nbytes, yes = 1;
socklen_t addrlen;
char buf[256];

void envia_msg(int sender_fd) {
    for (j = 0; j <= fdmax; j++) {
        if (FD_ISSET(j, &master)) {
            if ((j != sender_fd) && (j != sd)) {
                send(j, buf, nbytes, 0);
            }
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

void handle_connections() {
    read_fds = master;
    select(fdmax + 1, &read_fds, NULL, NULL, NULL);
    for (i = 0; i <= fdmax; i++) {
        if (FD_ISSET(i, &read_fds)) {
            if (i == sd) {
                newfd = accept(sd, (struct sockaddr*)&remoteaddr, &addrlen);
                FD_SET(newfd, &master);
                if (newfd > fdmax)
                    fdmax = newfd;
            } else {
                memset(&buf, 0, sizeof(buf));
                nbytes = recv(i, buf, sizeof(buf), 0);
                envia_msg(i);
            }
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        printf("Digite IP e Porta para este servidor\n");
        exit(1);
    }

    setup_socket(argv[1], argv[2]);

    for (;;) {
        handle_connections();
    }

    return 0;
}
