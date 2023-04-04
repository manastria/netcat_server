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

/**
 * Prints the usage instructions for the program.
 */
void print_usage() {
    printf("Usage: ./socket_server [-t|-u] [port]\n");
    printf("Options:\n");
    printf("  -t    Utiliser le protocole TCP (par défaut)\n");
    printf("  -u    Utiliser le protocole UDP\n");
    printf("  port  Numéro de port (par défaut: 3300)\n");
}

/**
 * Sends the current date and time, as well as the socket parameters, to the client.
 *
 * @param sockfd     The socket file descriptor.
 * @param clientaddr The client address structure.
 * @param clientlen  The length of the client address structure.
 * @param is_udp     True if the protocol is UDP, false if it is TCP.
 */
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

    // Parse command-line arguments
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

    // Create the socket
    sockfd = socket(AF_INET, is_udp ? SOCK_DGRAM : SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur lors de la création du socket");
        return 1;
    }

    // Set up the server address structure
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    // Bind the socket to the server address
    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("Erreur lors du bind");
        return 1;
    }

    // Start listening for incoming connections (TCP only)
    if (!is_udp && listen(sockfd, 5) < 0) {
        perror("Erreur lors de l'écoute");
        return 1;
    }

    printf("Serveur en écoute sur le port %d...\n", port);

    // Main server loop
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

        // Send the date, time, and socket parameters to the client
        send_time_and_socket_info(clientfd, &clientaddr, clientlen, is_udp);

        // Communication loop
        while (1) {
            char buffer[BUFFER_SIZE];
            ssize_t n;

            // Receive a message from the client
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

            // Handle client commands
            if (strcmp(trimmed_buffer, "quit") == 0 || strcmp(trimmed_buffer, "exit") == 0) {
                close(clientfd);
                close(sockfd);
                printf("Serveur arrêté.\n");
                return 0;
            } else if (strcmp(trimmed_buffer, "bye") == 0) {
                break;
            } else {
                // Send the date and the received message back to the client
                time_t now = time(NULL);
                struct tm tm_now = *localtime(&now);
                char time_str[64];
                strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", &tm_now);

                char response[BUFFER_SIZE];
                snprintf(response, sizeof(response), "%s | %s\n", time_str, trimmed_buffer);

                if (is_udp) {
                    sendto(clientfd, response, strlen(response), 0, (struct sockaddr *)&clientaddr, clientlen);
                } else {
                    send(clientfd, response, strlen(response), 0);
                }
            }
        }

        // Close the client socket (TCP only)
        if (!is_udp) {
            close(clientfd);
        }
    }

    // Close the server socket
    close(sockfd);
    return 0;
}

