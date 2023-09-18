#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>



#include "helpers.h"
#include "subscribers.h"

#define BUFLEN 1552

void register_id(int sockfd, char* id) {
    int rc;
    struct msgClientServer msg;
    msg.type = REGISTER_ID;
    strcpy(msg.id, id);
    rc = send(sockfd, &msg, sizeof(msg), 0);
    DIE(rc < 0, "send");
}
void subscribe(int sockfd, char* topic, int SF) {
    int rc;
    struct msgClientServer msg;
    msg.type = SUBSCRIBE;
    strcpy(msg.topic, topic);
    msg.sf = SF;
    rc = send(sockfd, &msg, sizeof(msg), 0);
    DIE(rc < 0, "send");
}
void unsubscribe(int sockfd, char* topic) {
    int rc;
    struct msgClientServer msg;
    msg.type = UNSUBSCRIBE;
    strcpy(msg.topic, topic);
    rc = send(sockfd, &msg, sizeof(msg), 0);
    DIE(rc < 0, "send");
}

void run_client(int sockfd, char* id) {
    char fileName[30] = "logs/client";
    strcat(fileName, id);
    strcat(fileName, ".log");

    FILE *f = fopen(fileName, "w");
    setbuf(f, NULL);

    register_id(sockfd, id);
    char *topic = (char*)malloc(50);
    int SF;

    struct messageTCP msg;
    int rc;

    struct pollfd poll_fds[2];
    int num_cons = 2;

    poll_fds[0].fd = sockfd;
    poll_fds[0].events = POLLIN;

    poll_fds[1].fd = STDIN_FILENO;
    poll_fds[1].events = POLLIN;

    char* commandType = (char*)malloc(20);



    while (1) {
        rc = poll(poll_fds, num_cons, -1);
        DIE(rc < 0, "poll");

        fprintf(f, "Poll returned\n");

        if (poll_fds[0].revents & POLLIN) {
            rc = recv(sockfd, &msg, sizeof(msg), 0);
            DIE(rc < 0, "recv");
            if (rc == 0) {
                printf("Server closed\n");
                return;
            }
            printMessageTCP(msg);

        } else if (poll_fds[1].revents & POLLIN) {
            char * buffer = (char*)malloc(BUFLEN);
            memset(buffer, 0, BUFLEN);
            size_t len;
            getline(&buffer,&len, stdin);
            buffer[strlen(buffer) - 1] = '\0';
            fprintf(f, "Command: %s\n", buffer);
            char *token = strtok(buffer, " ");
            strcpy(commandType, token);
            if (strcmp(commandType, "subscribe") == 0) {
                token = strtok(NULL, " ");
                strcpy(topic, token);
                token = strtok(NULL, " ");
                SF = atoi(token);
                fprintf(f, "Topic: %s, SF: %d\n", topic, SF);
                subscribe(sockfd, topic, SF);
                printf("Subscribed to topic.\n");
                fprintf(f, "Subscribed to %s\n", topic);
            } else if (strcmp(commandType, "unsubscribe") == 0) {
                token = strtok(NULL, " ");
                strcpy(topic, token);
                fprintf(f, "Topic: %s\n", topic);
                unsubscribe(sockfd, topic);
                printf("Unsubscribed from topic.\n");
            } else if (strcmp(commandType, "exit") == 0) {
                return;
            } else {
                fprintf(stderr, "Invalid command\n");
            }
        }
    }
    free (topic);
    free (commandType);
}

void printMessageTCP(struct messageTCP msg) {
  char sign = msg.payload[0];
  uint32_t aux = 0;
  uint16_t aux2 = 0;
  float aux3 = 0;;
  uint8_t exp = 0;
  switch (msg.data_type) {
    case 0:
      memcpy(&aux, msg.payload + 1, sizeof(uint32_t));
      aux = ntohl(aux);
      if (sign == 1) {
        aux = -aux;
      }

      printf("%s:%hu - %s - INT - %d\n", msg.ip, msg.port, msg.topic, aux);
      break;
    case 1:
        memcpy(&aux2, msg.payload, sizeof(uint16_t));
        aux2 = ntohs(aux2);


      printf("%s:%hu - %s - SHORT_REAL - %d.%d%d\n", msg.ip, msg.port, msg.topic, (int)aux2/100, (int)(aux2/10)%10, (int)aux2%10);
      break;
    case 2: 
        memcpy(&aux, msg.payload + 1, sizeof(uint32_t));
        
        aux = ntohl(aux);
        memcpy(&exp, msg.payload + 1 + sizeof(uint32_t), sizeof(uint8_t));
        
        aux3 = aux/(pow(10,exp));
        if(sign == 1) 
            aux3 = -aux3;
      printf("%s:%hu - %s - FLOAT - %f\n", msg.ip, msg.port, msg.topic, aux3);
      break;
    case 3:
      printf("%s:%hu - %s - STRING - %s\n", msg.ip, msg.port, msg.topic, msg.payload);
      break;

    default:
        break;

  }
}

int main(int argc, char* argv[]) {
    setvbuf(stdin, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);
    int sockfd = -1;
    if (argc < 4) {
        printf("Usage: ./subscriber <ID_Client> <IP_Server> <Port_Server>\n");
        return -1;
    }
    uint16_t port;
  int rc = sscanf(argv[3], "%hu", &port);
  DIE(rc != 1, "Given port is invalid");

  // Obtinem un socket TCP pentru conectarea la server
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  DIE(sockfd < 0, "socket");

  // Completăm in serv_addr adresa serverului, familia de adrese si portul
  // pentru conectare
  struct sockaddr_in serv_addr;
  socklen_t socket_len = sizeof(struct sockaddr_in);

  memset(&serv_addr, 0, socket_len);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  rc = inet_pton(AF_INET, argv[2], &serv_addr.sin_addr.s_addr);
  DIE(rc <= 0, "inet_pton");

  // Ne conectăm la server
  rc = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
  DIE(rc < 0, "connect");


  run_client(sockfd, argv[1]);

  // Inchidem conexiunea si socketul creat
  close(sockfd);

  return 0;
}