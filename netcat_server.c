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
#include <ifaddrs.h>
#include <getopt.h>
#include <netdb.h>
#include <signal.h>
#include <pthread.h>


#define DEFAULT_PORT 3300
#define BUFFER_SIZE 1024


// Déclare le descripteur de fichier du socket comme variable globale
int sockfd;

void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("Received signal %d, closing server...\n", sig);
        close(sockfd);
        exit(0);
    }
}

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


void display_ips() {
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    printf("IPv4 addresses:\n");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\t%s: %s\n", ifa->ifa_name, host);
        }
    }

    printf("IPv6 addresses:\n");
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;
        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET6) {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in6),
                            host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0) {
                printf("getnameinfo() failed: %s\n", gai_strerror(s));
                exit(EXIT_FAILURE);
            }
            printf("\t%s: %s\n", ifa->ifa_name, host);
        }
    }

    freeifaddrs(ifaddr);
}

void parse_arguments(int argc, char *argv[], bool *is_udp, int *port) {
    int c;

    while ((c = getopt(argc, argv, "tu:p:")) != -1) {
        switch (c) {
            case 't':
                *is_udp = false;
                break;
            case 'u':
                *is_udp = true;
                break;
            case 'p':
                *port = atoi(optarg);
                break;
            default:
                print_usage();
                exit(1);
        }
    }
}


int create_socket(bool is_udp, int port) {
    int sockfd = socket(AF_INET, is_udp ? SOCK_DGRAM : SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("Erreur lors de la création du socket");
        exit(1);
    }

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
        perror("Erreur lors du bind");
        exit(1);
    }

    if (!is_udp && listen(sockfd, 5) < 0) {
        perror("Erreur lors de l'écoute");
        exit(1);
    }

    return sockfd;
}

void handle_client_communication(int clientfd, bool is_udp, struct sockaddr_in clientaddr, socklen_t clientlen) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, client_ip, INET_ADDRSTRLEN);
    int client_port = ntohs(clientaddr.sin_port);
    printf("Received connexion from client %s:%d\n", client_ip, client_port);

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
            printf("Serveur arrêté.\n");
            exit(0);
        } else if (strcmp(trimmed_buffer, "bye") == 0) {
            break;
        } else {
            printf("Received message from client %s:%d: %s\n", client_ip, client_port, trimmed_buffer);

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

            printf("Sent message to client %s:%d\n", client_ip, client_port);
            printf("Sent message to client %s:%d: %s\n", client_ip, client_port, response);
        }
    }

    if (!is_udp) {
        close(clientfd);
    }
}

void* client_handler(void* arg) {
    int clientfd = *(int*)arg;
    struct sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    getpeername(clientfd, (struct sockaddr*)&clientaddr, &clientlen);
    handle_client_communication(clientfd, true, clientaddr, clientlen);
    close(clientfd);
    pthread_exit(NULL);
}

void run_server(bool is_udp, int port) {
    int sockfd = create_socket(is_udp, port);
    printf("Serveur en écoute sur le port %d...\n", port);

    while (1) {
        int clientfd;
        struct sockaddr_in clientaddr;
        socklen_t clientlen = sizeof(clientaddr);

        if (is_udp) {
            clientfd = sockfd;
        } else {
            clientfd = accept(sockfd, (struct sockaddr *)&clientaddr, &clientlen);
            if (clientfd < 0) {
                perror("Erreur lors de l'acceptation de la connexion");
                continue;
            }
        }

        // Crée un thread pour gérer la connexion
        pthread_t thread_id;
        int* new_clientfd = malloc(sizeof(int));
        *new_clientfd = clientfd;

        if (pthread_create(&thread_id, NULL, client_handler, (void*)new_clientfd) < 0) {
            perror("Erreur lors de la création du thread");
            close(clientfd);
            free(new_clientfd);
            continue;
        }

        // Détache le thread pour libérer les ressources
        pthread_detach(thread_id);
    }

    close(sockfd);
}



int main(int argc, char *argv[]) {
    bool is_udp = false;
    int port = DEFAULT_PORT;

    // Enregistre le gestionnaire de signal pour le signal SIGINT (CTRL+C)
    signal(SIGINT, signal_handler);

    sockfd = create_socket(is_udp, port);

    // Enregistre le gestionnaire de signal pour le signal SIGINT (CTRL+C)
    signal(SIGINT, signal_handler);

    sockfd = create_socket(is_udp, port);

    parse_arguments(argc, argv, &is_udp, &port);
    display_ips();
    run_server(is_udp, port);

    return 0;
}
