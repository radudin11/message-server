#ifndef _HELPERS_H
#define _HELPERS_H 1

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>


/*
 * Macro de verificare a erorilor
 * Exemplu:
 * 		int fd = open (file_name , O_RDONLY);
 * 		DIE( fd == -1, "open failed");
 */

#define SERVER_ADDR 0x100007f  // 
#define MAX_PACKAGE_SIZE 1552
#define MSG_NUM 100
#define TOPIC_NUM 50

#define REGISTER_ID 0
#define SUBSCRIBE 1
#define UNSUBSCRIBE 2

#define DIE(assertion, call_description)                                       \
  do {                                                                         \
    if (assertion) {                                                           \
      fprintf(stderr, "(%s, %d): ", __FILE__, __LINE__);                       \
      perror(call_description);                                                \
      exit(EXIT_FAILURE);                                                      \
    }                                                                          \
  } while (0)


struct __attribute__((packed)) messageUPD {
  char topic[50];
  uint8_t data_type;
  char payload[1500];
};

struct __attribute__((packed)) messageTCP {
  char topic[50];
  uint8_t data_type;
  char payload[1500];
  char ip[16];
  uint16_t port;
};

struct topic {
  char name[50];
  char sf;
}__attribute__((packed));

struct client {
  char id[10];
  char active;
  struct messageTCP messages[MSG_NUM];
  int num_messages;
  struct topic topics[TOPIC_NUM];
  int num_topics;
  int fd;
  char* ip;
  uint16_t port;
}__attribute__((packed));

struct msgClientServer {
  char type;
  char id[10];
  char topic[50];
  char sf;
}__attribute__((packed));


void printMessageTCP(struct messageTCP msg);

#endif