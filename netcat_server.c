#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define DEFAULT_PORT 3300
#define BUFFER_SIZE 1024

void print_usage() {
    printf("Usage: ./socket_server [-t|-u] [port]\n");
    printf("Options:\n");
    printf("  -t    Utiliser le protocole TCP (par défaut)\n");
    printf("  -u    Utiliser le protocole UDP\n");
    printf("  port  Numéro de port (par défaut: 3300)\n");
}

void send_time_and_socket_info(int sockfd, struct sockaddr_in *clientaddr, socklen_t clientlen, bool is_udp) {
    char buffer[BUFFER_SIZE];
    time_t now = time(NULL);
    struct tm tm_now = *localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_now);

    snprintf(buffer, sizeof(buffer), "%s | Adresse IP: %s, Port: %d, Protocole: %s\n",
             time_str, inet_ntoa(clientaddr->sin_addr), ntohs(clientaddr->sin_port),
             is_udp ? "UDP" : "TCP");

    if (is_udp) {
        sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)clientaddr, clientlen);
    } else {
        send(sockfd, buffer, strlen(buffer), 0);
    }
}

int main(int argc, char *argv[]) {
    int sockfd;
    bool is_udp = false;
    int port = DEFAULT_PORT;
    struct sockaddr_in serveraddr, clientaddr;
    socklen_t clientlen = sizeof(clientaddr);

    if (argc > 1) {
        if (strcmp(argv[1], "-t") == 0) {
            is_udp = false;
        } else if (strcmp(argv[1], "-u") == 0) {
            is_udp = true;
        } else {
            print_usage();
            return 1;
        }
        if (argc > 2) {
            port = atoi(argv[2]);
        }
    }

    sockfd = socket(AF_INET, is_udp ? SOCK_DGRAM : SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur lors de la création du socket");
        return 1;
    }

    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("Erreur lors du bind");
        return 1;
    }

    if (!is_udp && listen(sockfd, 5) < 0) {
        perror("Erreur lors de l'écoute");
        return 1;
    }

    printf("Serveur en écoute sur le port %d...\n", port);

    while (1) {
        int clientfd;
        if (is_udp) {
            clientfd = sockfd;
        } else {
            clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen);
            if (clientfd < 0) {
                perror("Erreur lors de l'acceptation de la connexion");
                continue;
            }
        }

        send_time_and_socket_info(clientfd, &clientaddr, clientlen, is_udp);

        while (1) {
            char buffer[BUFFER_SIZE];
            ssize_t n;

            if (is_udp) {
                n = recvfrom(clientfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&clientaddr, &clientlen);
            } else {
                n = recv(clientfd, buffer, BUFFER_SIZE, 0);
            }

            if (n <= 0) {
                break;
            }

            buffer[n] = '\0';
            char *trimmed_buffer = strtok(buffer, "\r\n");

            if (strcmp(trimmed_buffer, "quit") == 0 || strcmp(trimmed_buffer, "exit") == 0) {
                close(clientfd);
                close(sockfd);
                printf("Serveur arrêté.\n");
                return 0;
            } else if (strcmp(trimmed_buffer, "bye") == 0) {
                break;
            } else {
                time_t now = time(NULL);
                struct tm tm_now = *localtime(&now);
                char time_str[64];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_now);

                snprintf(buffer, sizeof(buffer), "%s | %s\n", time_str, trimmed_buffer);

                if (is_udp) {
                    sendto(clientfd, buffer, strlen(buffer), 0, (struct sockaddr *)&clientaddr, clientlen);
                } else {
                    send(clientfd, buffer, strlen(buffer), 0);
                }
            }
        }

        if (!is_udp) {
            close(clientfd);
        }
    }

    close(sockfd);
    return 0;
}

