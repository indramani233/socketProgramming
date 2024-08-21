#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "mysocket.h"
int main()
{
    int sock;                                    // Socket Descriptor
    struct sockaddr_in client_addr, server_addr; // Structure to store address
    char buffer[100] = "hello from server\n";

    sock = my_socket(AF_INET, SOCK_MyTCP, 0);

    if (sock < 0)
    {
        perror("Could not create socket\n");
        exit(0);
    }
    int tosend = 12;
    // Filling the server address details
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(20000);

    // Binding server address to socket
    if (my_bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Could not bind local address\n");
        exit(0);
    }

    printf("Server is up and running....\n");

    my_listen(sock, 5);

    int cli_addr_len = sizeof(client_addr);
    // Accept connection request
    int newsock = my_accept(sock, (struct sockaddr *)&client_addr, &cli_addr_len);

    if (newsock < 0)
    {
        perror("Could not accept\n");
        exit(0);
    }

    // Send string back to the client
    for (int i = 0; i < 20; i++)
    {
        tosend = i * i;
        my_send(newsock, &tosend, sizeof(tosend), 0);
        printf("Done %d\n", i);
        sleep(5);
    }

    // Close the socket
    my_close(newsock);

    return 0;
}