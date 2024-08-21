#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "mysocket.h"
int main()
{
    int sock;                       // Socket Descriptor
    struct sockaddr_in server_addr; // Structure to store address
    char buffer[100];

    sock = my_socket(AF_INET, SOCK_MyTCP, 0);
    if (sock < 0)
    {
        perror("Could not create socket\n");
        exit(0);
    }

    // Filling the server address details
    server_addr.sin_family = AF_INET;
    inet_aton("127.0.0.1", &server_addr.sin_addr);
    server_addr.sin_port = htons(20000);

    // Binding the server address to socket
    if (my_connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Could not connect\n");
        exit(0);
    }
    printf("Connected\n");
    // for (int i = 0; i < 100; i++)
    //     buffer[i] = '\0';

    // Recieve messege from the server and print it
    int recvint;
    for (int i = 0; i < 20; i++)
    {

        my_recv(sock, &recvint, 100, 0);
        printf("Done %d\n", recvint);
        sleep(5);
    }
    // Close socket
    my_close(sock);
    return 0;
}