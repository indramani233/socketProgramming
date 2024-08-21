#ifndef MYSOCKET_H
#define MYSOCKET_H
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SOCK_MyTCP 100

int my_socket(int domain, int type, int protocol);
int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int my_listen(int sockfd, int backlog);
int my_accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen);
int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
ssize_t my_send(int sockfd, const void *buf, size_t len, int flags);
ssize_t my_recv(int sockfd, void *buf, size_t len, int flags);
int my_close(int fd);

void *func_R(void *param);
void *func_S(void *param);

typedef struct
{
    void *msg;
    ssize_t msg_size;
} Message;

typedef struct
{
    Message table[10];
    ssize_t counter;
    int in;
    int out;
} Table;

#endif