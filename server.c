#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/timerfd.h>
#include <math.h>

#include "helpers.h"

#define MAX_CONNECTIONS 100
#define BUFLEN 1552
#define MAX_CLIENTS 100
#define MAX_TOPICS 100

struct client clienti[MAX_CLIENTS];
int num_clients = 0;


void sendMessage(struct messageTCP msg) {
    // printf("send message\n");
    // cauta toti clientii care au topicul
    for (int i = 0; i < num_clients; i++) {
        for (int j = 0; j < clienti[i].num_topics; j++) {
            if (strcmp(clienti[i].topics[j].name, msg.topic) == 0) {
                // daca este activ trimite mesajul
                if (clienti[i].active == 1) {
                    int rc = send(clienti[i].fd, &msg, sizeof(msg), 0);
                    DIE(rc < 0, "send");
                } else {
                    // daca nu este activ si are sf = 1, salveaza mesajul
                    if (clienti[i].topics[j].sf == 1) {
                        clienti[i].messages[clienti[i].num_messages] = msg;
                        clienti[i].num_messages++;
                    }
                }
            }
        }
    }
}

void printClient(struct client client) {
    printf("Client: %s\n", client.id);
    printf("Num topics: %d\n", client.num_topics);
    for (int i = 0; i < client.num_topics; i++) {
        printf("Topic: %s\n", client.topics[i].name);
        printf("SF: %d\n\n", client.topics[i].sf);

    }
    printf("Num messages: %d\n", client.num_messages);
    for (int i = 0; i < client.num_messages; i++) {
        printf("Message: %s\n\n", client.messages[i].topic);
    }

    printf("Status: %s\n", client.active == 1 ? "active" : "inactive");
    printf("FD: %d\n\n", client.fd);

}


void run_server(int TCPfd, int UDPfd) {

    struct pollfd poll_fds[MAX_CONNECTIONS];
    int num_cons = 1;
    int rc;
    struct sockaddr_in cli_addr;
    socklen_t cli_len = sizeof(cli_addr);

    struct messageTCP msg_tcp;

    // Setam socket-ul listenfd pentru ascultare
    rc = listen(TCPfd, MAX_CONNECTIONS);
    DIE(rc < 0, "listen");

    // se adauga noul file descriptor (socketul pe care se asculta conexiuni) in
    // multimea read_fds
    poll_fds[0].fd = TCPfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = UDPfd;
    poll_fds[1].events = POLLIN;

    poll_fds[2].fd = STDIN_FILENO;
    poll_fds[2].events = POLLIN;

    num_cons = 3;


    FILE * serverLog = fopen("logs/server.log", "a");
    if (serverLog == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }

    while (1) {

        rc = poll(poll_fds, num_cons, -1);
        DIE(rc < 0, "poll");

        for (int i = 0; i < num_cons; i++) {
            if (poll_fds[i].revents & POLLIN) {
                if (poll_fds[i].fd == TCPfd) {
                    fprintf(serverLog, "New client connected\n");
                    // a venit o cerere de conexiune pe socketul inactiv (cel cu listen),
                    // pe care serverul o accepta
                    struct sockaddr_in cli_addr;
                    socklen_t cli_len = sizeof(cli_addr);
                    int newsockfd =
                        accept(TCPfd, (struct sockaddr *)&cli_addr, &cli_len);
                    DIE(newsockfd < 0, "accept");

                    // se adauga noul socket intors de accept() la multimea descriptorilor
                    // de citire
                    poll_fds[num_cons].fd = newsockfd;
                    poll_fds[num_cons].events = POLLIN;
                    num_cons++;

                    clienti[num_clients].fd = newsockfd;
                    clienti[num_clients].active = 1;
                    clienti[num_clients].num_topics = 0;
                    clienti[num_clients].num_messages = 0;
                    clienti[num_clients].ip = inet_ntoa(cli_addr.sin_addr);
                    clienti[num_clients].port = ntohs(cli_addr.sin_port);
                    num_clients++;

                } else {
                    if (poll_fds[i].fd == UDPfd) {
                        // am primit date de la clientii UDP
                        memset(&cli_addr, 0, sizeof(cli_addr));

                        char buffer[BUFLEN];
                        memset(buffer, 0, BUFLEN);


                        rc = recvfrom(UDPfd, buffer, BUFLEN, 0,
                            (struct sockaddr *)&cli_addr, &cli_len);
                        DIE(rc < 0, "recvfrom");

                        // construiesc mesajul de trimis prin TCP
                        memset(&msg_tcp, 0, sizeof(msg_tcp));

                        strncpy(msg_tcp.topic, buffer, 50);
                        msg_tcp.data_type = buffer[50];
                        memcpy(&msg_tcp.payload, buffer + 51, 1500);
                        msg_tcp.port = ntohs(cli_addr.sin_port);
                        strcpy(msg_tcp.ip, inet_ntoa(cli_addr.sin_addr));


                        // trimit mesajul prin TCP
                        sendMessage(msg_tcp);
                    } else {
                        fprintf(serverLog, "New message received\n");
                        if (poll_fds[i].fd == STDIN_FILENO) {
                            // am primit date de la tastatura
                            char buffer[BUFLEN];
                            memset(buffer, 0, BUFLEN);

                            fgets(buffer, BUFLEN - 1, stdin);

                            if (strncmp(buffer, "exit", 4) == 0) {
                                // am primit comanda exit
                                for (int j = 0; j < num_clients; j++) {
                                    if (poll_fds[j].fd != STDIN_FILENO) {
                                        close(poll_fds[j].fd);
                                        poll_fds[j].fd = -1;
                                    }
                                }
                                return;
                            } else {
                                fprintf(stderr, "Am primit de la tastatura comanda: %s\n", buffer);
                            }
                        } else {
                            // am primit date pe unul din socketii cu care vorbesc cu clientii
                            // TCP
                            struct msgClientServer msg;
                            memset(&msg, 0, sizeof(msg));

                            rc = recv(poll_fds[i].fd, &msg, sizeof(msg), 0);
                            DIE(rc < 0, "recv");

                            if (rc == 0) {
                                char* id;
                                // conexiunea s-a inchis

                                // seteaza clientul ca inactiv
                                for (int j = 0; j < num_clients; j++) {
                                    if (clienti[j].fd == poll_fds[i].fd) {
                                        clienti[j].active = 0;
                                        id = clienti[j].id;
                                    }
                                }


                                close(poll_fds[i].fd);
                                // se scoate din multimea de citire socketul inchis
                                for (int j = i; j < num_cons - 1; j++) {
                                    poll_fds[j] = poll_fds[j + 1];
                                }
                                // Client <ID_CLIENT> disconnected
                                printf("Client %s disconnected.\n", id);

                                num_cons--;
                                i--;

                                
                            } else {
                                int rc;
                                int found = 0;
                                switch (msg.type)
                                {
                                case REGISTER_ID:
                                    // verificare id unic
                                    found = 0;
                                    for (int j = 0; j < num_clients; j++) {
                                        if (strcmp(clienti[j].id, msg.id) == 0) {
                                            // verifica daca clientul este activ
                                            if (clienti[j].active == 1) {
                                                // Client <ID_CLIENT> already connected.
                                                printf("Client %s already connected.\n", msg.id);
                                                // inchide conexiunea
                                                found = poll_fds[i].fd;
                                                close(poll_fds[i].fd);
                                                // scoate din multimea de citire socketul inchis
                                                for (int k = i; k < num_cons - 1; k++) {
                                                    poll_fds[k] = poll_fds[k + 1];
                                                }
                                                num_cons--;
                                                i--;

                                                break;
                                            }

                                            // daca nu este activ, il seteaza ca activ
                                            clienti[j].active = 1;

                                            // seteaza fd-ul clientului
                                            clienti[j].fd = poll_fds[i].fd;

                                            // afiseaza conexiunea
                                            printf("New client %s connected from %s:%d.\n", clienti[j].id,
                                                    clienti[j].ip, clienti[j].port);

                                            // trimite mesajele retinute
                                            for (int k = 0; k < clienti[j].num_messages; k++) {
                                                rc = send(clienti[j].fd, &clienti[j].messages[k], sizeof(clienti[j].messages[k]), 0);
                                                DIE(rc < 0, "send");

                                                // printf("Am trimis mesajul %s catre clientul cu id-ul %s\n", clienti[j].messages[k].payload, clienti[j].id);
                                            }

                                            // elimina mesajele retinute
                                            clienti[j].num_messages = 0;

                                            found = 1;

                                            break;
                                        }
                                    }

                                    if (found) {
                                        if (found == 1) {
                                            // a fost gasit si era inactiv
                                            found = poll_fds[i].fd;
                                        }
                                        // elimina clientul creat la primirea conexiunii
                                        for (int k = num_clients - 1; k >=0 ; k--) {
                                            if (clienti[k].fd == found) {
                                                for (int l = k; l < num_clients - 1; l++) {
                                                    clienti[l] = clienti[l + 1];
                                                }
                                                num_clients--;
                                                break;
                                            }
                                        }
                                        break;
                                    } else {
                                        for (int j = 0; j < num_clients; j++) {
                                            if (clienti[j].fd == poll_fds[i].fd) {
                                                strcpy(clienti[j].id, msg.id);
                                                clienti[j].active = 1;
                                                 printf("New client %s connected from %s:%d.\n", clienti[num_clients - 1].id,
                                                    clienti[num_clients - 1].ip, clienti[num_clients - 1].port);
                                                break;
                                            }
                                        }
                                    }

                                    // cauta clientul in lista de clienti si seteaza id-ul
                                    break;
                                case SUBSCRIBE:
                                    // adauga topicul in lista de topicuri a clientului
                                    for (int j = 0; j < num_clients; j++) {
                                        if (clienti[j].fd == poll_fds[i].fd) {
                                            // verific daca topicul exista deja
                                            int found = 0;
                                            for (int k = 0; k < clienti[j].num_topics; k++) {
                                                if (strcmp(clienti[j].topics[k].name, msg.topic) == 0) {
                                                    found = 1;
                                                    break;
                                                }
                                            }
                                            if (found == 0) {
                                                strcpy(clienti[j].topics[clienti[j].num_topics].name, msg.topic);
                                                clienti[j].topics[clienti[j].num_topics].sf = msg.sf;
                                                clienti[j].num_topics++;
                                            } else {
                                                fprintf(stderr, "Clientul %s este deja abonat la topicul %s\n", clienti[j].id, msg.topic);
                                            }
                                        }
                                    }
                                    break;
                                case UNSUBSCRIBE:
                                    // sterge topicul din lista de topicuri a clientului
                                    for (int j = 0; j < num_clients; j++) {
                                        if (clienti[j].fd == poll_fds[i].fd) {
                                            // verific daca topicul exista
                                            int found = 0;
                                            for (int k = 0; k < clienti[j].num_topics; k++) {
                                                if (strcmp(clienti[j].topics[k].name, msg.topic) == 0) {
                                                    found = 1;
                                                    // sterg topicul
                                                    for (int l = k; l < clienti[j].num_topics - 1; l++) {
                                                        clienti[j].topics[l] = clienti[j].topics[l + 1];
                                                    }
                                                    clienti[j].num_topics--;
                                                    break;
                                                }
                                            }
                                            if (found == 0) {
                                                fprintf(stderr, "Clientul %s nu este abonat la topicul %s\n", clienti[j].id, msg.topic);
                                            }
                                        }
                                    }
                                    break;
                                default:
                                    break;
                                }  
                            }
                        }
                    }
                }
            }
        }
    }
}


int getUPDfd(int port) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    DIE(fd < 0, "socket");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int rc = bind(fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind");

    return fd;
}

int getTCPfd(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    DIE(fd < 0, "socket");

    struct sockaddr_in serv_addr;
    socklen_t socket_len = sizeof(struct sockaddr_in);

    int enable = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

    memset(&serv_addr, 0, socket_len);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY;

    int rc = bind(fd, (const struct sockaddr *)&serv_addr, sizeof(serv_addr));
    DIE(rc < 0, "bind");

    return fd;
}


int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);

    if (argc != 2) {
        printf("\n Usage: %s <PORT>\n", argv[0]);
        return 1;
    }

    // Parsam port-ul ca un numar
    uint16_t port;
    int rc = sscanf(argv[1], "%hu", &port);
    DIE(rc != 1, "Given port is invalid");

    // Obtinem un socket UPD pentru topicuri si unul TCP pentru clienti
    int TCPfd = getTCPfd(port);
    DIE(TCPfd < 0, "socket");

    int UPDfd = getUPDfd(port);
    DIE(UPDfd < 0, "socket");

    run_server(TCPfd, UPDfd);

    // Inchidem listenfd
    close(TCPfd);
    close(UPDfd);

    return 0;
}