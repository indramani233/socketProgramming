/*
Assignment 4
Anuj Kakde (20CS30005)
Sarthak Nikumbh (20CS30035)
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>

#define MAXLEN 50

char *recieve_data(int sock, char *buffer, int *len)
{
    // Recieve result in chunks of 50 bytes
    int totalBytesRecieved = 0;
    int bytesRecieved;
    char *data = (char *)malloc(1 * sizeof(char));
    while (1)
    {

        for (int i = 0; i < MAXLEN; i++)
        {
            buffer[i] = '\0';
        }

        bytesRecieved = recv(sock, buffer, MAXLEN, 0);
        if (bytesRecieved == 0)
        {
            printf("Connection closed by the server\n");
            exit(0);
        }
        if (bytesRecieved < 0)
        {
            perror("Could not recieve\n");
            exit(0);
        }
        totalBytesRecieved += bytesRecieved;

        // Reallocate space
        data = (char *)realloc(data, totalBytesRecieved * sizeof(char));
        int j = 0;
        for (int i = totalBytesRecieved - bytesRecieved; i < totalBytesRecieved; i++)
        {
            data[i] = buffer[j++];
        }

        char *p = strstr(data, "\r\n\r\n");
        if (p)
        {
            break;
        }
    }
    *len = totalBytesRecieved;
    return data;
}

char *get_ip(char *url)
{
    int len = 0;
    int flag = 0;
    for (int i = 0; i < strlen(url); i++)
    {
        if (flag > 2)
        {
            break;
        }
        if (flag == 2)
        {
            len++;
        }
        if (url[i] == '/')
        {
            flag++;
        }
    }

    char *ip = (char *)malloc(len * sizeof(char));

    int index = 0;
    flag = 0;
    for (int i = 0; i < strlen(url); i++)
    {
        if (flag > 2)
        {
            break;
        }
        if (flag == 2)
        {
            ip[index] = url[i];
            index++;
        }
        if (url[i] == '/')
        {
            flag++;
        }
    }

    // printf("len:%d\n", len);

    ip[len - 1] = '\0';
    return ip;
}

char *get_port(char *url)
{
    int len = 0;
    int index = -1;
    for (int i = strlen(url); i >= 7; i--)
    {
        if (url[i] == ':')
        {
            index = i;
        }
    }
    if (index == -1)
    {
        char *ip = (char *)malloc(3 * sizeof(char));
        ip[0] = '8';
        ip[1] = '0';
        ip[2] = '\0';
        return ip;
    }
    for (int i = index + 1; i < index + MAXLEN; i++)
    {
        if (url[i] == '\0')
        {
            break;
        }
        len++;
    }
    len++;
    char *port = (char *)malloc(len * sizeof(char));
    for (int i = 0; i < len - 1; i++)
    {
        port[i] = url[index + 1 + i];
    }
    port[len - 1] = '\0';
    return port;
}

int main()
{

    while (1)
    {
        printf("\033[1;33mMyOwnBrowser> \033[0m");

        // Take command as input
        size_t len = 0;
        char *input = NULL;
        getline(&input, &len, stdin);
        input[strlen(input) - 1] = '\0';
        char *input_dup = strdup(input);

        char *token;

        // get the first token
        token = strtok(input_dup, " ");
        if (strcmp(token, "QUIT") == 0)
        {
            break;
        }

        // Handle get command
        else if (strcmp(token, "GET") == 0)
        {

            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("Error: Invalid Command\n");
                continue;
            }
            char *url = token;
            token = strtok(NULL, " ");
            if (token != NULL)
            {
                printf("Error: Invalid Command\n");
                continue;
            }

            char *ip = get_ip(url);
            char *port = get_port(url);

            // printf("IP:%s\nPort:%s\n", ip, port);

            // // Create socket
            int sock;
            struct sockaddr_in server_addr;
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                perror("Could not create socket");
                exit(0);
            }

            server_addr.sin_family = AF_INET;
            inet_aton(ip, &server_addr.sin_addr);
            server_addr.sin_port = htons(atoi(port));

            if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            {
                perror("Could not connect to the server");
                exit(0);
            }

            printf("Connected to Server....\n");

            // Send HTTP request

            // Get path
            char *path = (char *)malloc(strlen(input) * sizeof(char));
            char *file_name = (char *)malloc(strlen(input) * sizeof(char));
            strcpy(path, "GET /");
            int flag = 0;
            int index = 5;
            for (int i = 0; url[i] != '\0'; i++)
            {
                if (flag > 3)
                {
                    if (url[i] == ':')
                    {
                        break;
                    }
                    path[index] = url[i];
                    file_name[index - 5] = url[i];
                    index++;
                }
                if (url[i] == '/' || url[i] == ':')
                {
                    flag++;
                }
            }
            file_name[index] = '\0';
            path[index] = '\0';

            // Get accept type
            int last_dot = -1;
            int len = strlen(path);
            for (int i = 0; i < len; i++)
            {
                if (path[i] == '.')
                {
                    last_dot = i;
                }
            }
            char accept[20] = "";
            int content_cases = -1;
            if (path[last_dot + 1] == 'h' && path[last_dot + 2] == 't' && path[last_dot + 3] == 'm' && path[last_dot + 4] == 'l' && (path[last_dot + 5] == '\0' || path[last_dot + 5] == ':'))
            {
                strcpy(accept, "text/html");
                content_cases = 0;
            }
            else if (path[last_dot + 1] == 'p' && path[last_dot + 2] == 'd' && path[last_dot + 3] == 'f' && (path[last_dot + 4] == '\0' || path[last_dot + 4] == ':'))
            {
                strcpy(accept, "application/pdf");
                content_cases = 1;
            }
            else if (path[last_dot + 1] == 'j' && path[last_dot + 2] == 'p' && path[last_dot + 3] == 'g' && (path[last_dot + 4] == '\0' || path[last_dot + 4] == ':'))
            {
                strcpy(accept, "image/jpeg");
                content_cases = 2;
            }
            else
            {
                strcpy(accept, "text/*");
                content_cases = 3;
            }

            strncat(path, " HTTP/1.1", 10);

            char accept_lan[20] = "en-US,en;q=0.7";

            time_t now, now_minus_2;
            time(&now);
            now_minus_2 = now - (2 * 24 * 60 * 60);
            char if_modified_since[50];
            char date[50];
            strftime(if_modified_since, sizeof(if_modified_since), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now_minus_2));
            strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));

            char header[5000] = "";
            char delimieter[5] = "\r\n";
            strncat(header, path, strlen(path) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Host: ", 7);
            strncat(header, ip, strlen(ip) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Connection: close", 20);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Date: ", 7);
            strncat(header, date, strlen(date) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Accept: ", 9);
            strncat(header, accept, strlen(accept) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Accept-Language: ", 20);
            strncat(header, accept_lan, strlen(accept_lan) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "If-Modified-Since: ", 20);
            strncat(header, if_modified_since, strlen(if_modified_since) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);

            printf("Sending: \n%s", header);

            if (send(sock, header, strlen(header), 0) <= 0)
            {
                perror("Server Down!");
                exit(0);
            }

            // Read http response
            char buffer[MAXLEN];

            struct pollfd pfd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            pfd.fd = sock;

            // Wait for maximum 3 seconds
            len = -1;
            int poll_return = poll(&pfd, 1, 3000);
            char *response = NULL;
            if (poll_return > 0)
            {
                response = recieve_data(sock, buffer, &len);
            }
            else if (poll_return < 0)
            {
                perror("Poll failed!\n");
                exit(0);
            }
            else
            {
                printf("Timed Out: No response by the server\n");
                continue;
            }

            // printf("Response: %s", response);
            //
            //
            printf("Response: \n");
            char *end = strstr(response, "\r\n\r\n");
            for (int i = 0; i < (end - response) + 4; i++)
            {
                printf("%c", response[i]);
            }
            char *response_dup = strdup(response);

            // Get Status
            token = strtok(response_dup, "\r\n");
            char *code = strtok(token, " ");
            code = strtok(NULL, " ");
            if (strcmp(code, "400") == 0)
            {
                printf("Bad Request\n");
            }
            else if (strcmp(code, "403") == 0)
            {
                printf("Forbidden\n");
            }
            else if (strcmp(code, "404") == 0)
            {
                printf("Not Found\n");
            }
            else if (strcmp(code, "200") != 0)
            {
                printf("Unknown Error Code: %s\n", code);
            }

            // Get Content Length and Content Type

            char content_type[20] = "";
            char content_length[100] = "";
            char *response_dup2 = strdup(response);
            token = strtok(response_dup2, "\r\n");
            // Walk through other tokens
            while (token != NULL)
            {
                char *temp1 = strstr(token, "Content-Length:");
                if (temp1)
                {
                    strcpy(content_length, temp1 + 16);
                }
                char *temp2 = strstr(token, "Content-Type:");
                if (temp2)
                {
                    strcpy(content_type, temp2 + 14);
                }
                token = strtok(NULL, "\r\n");
            }

            if (content_type == NULL || content_length == NULL || strlen(content_type) <= 0 || strlen(content_length) <= 0)
            {
                printf("Error occured while parsing for Content-Length/Content-Type\n");
            }
            // printf("Content-Length:%s\nContent-Type:%s\n", content_length, content_type);

            // Check content type and give appropriate extension
            last_dot = strlen(file_name);
            for (int i = 0; i < strlen(file_name); i++)
            {
                if (file_name[i] == '.')
                {
                    last_dot = i;
                }
            }
            if (strcmp(content_type, "text/html") == 0)
            {
                content_cases = 0;
                file_name[last_dot++] = '.';
                file_name[last_dot++] = 'h';
                file_name[last_dot++] = 't';
                file_name[last_dot++] = 'm';
                file_name[last_dot++] = 'l';
                file_name[last_dot++] = '\0';
            }
            else if (strcmp(content_type, "application/pdf") == 0)
            {
                content_cases = 1;
                file_name[last_dot++] = '.';
                file_name[last_dot++] = 'p';
                file_name[last_dot++] = 'd';
                file_name[last_dot++] = 'f';
                file_name[last_dot++] = '\0';
            }
            else if (strcmp(content_type, "image/jpeg") == 0)
            {
                content_cases = 2;
                file_name[last_dot++] = '.';
                file_name[last_dot++] = 'j';
                file_name[last_dot++] = 'p';
                file_name[last_dot++] = 'g';
                file_name[last_dot++] = '\0';
            }
            else
            {
                content_cases = 3;
                // file_name[last_dot++] = '.';
                // file_name[last_dot++] = 't';
                // file_name[last_dot++] = 'x';
                // file_name[last_dot++] = 't';
                // file_name[last_dot++] = '\0';
            }

            // printf("fileName: %s\n", file_name);

            // Recieve body
            FILE *fp;
            index = -1;
            for (int i = 0; i < strlen(file_name); i++)
            {
                if (file_name[i] == '/')
                {
                    index = i;
                }
            }
            index++;
            // printf("fileName: %s\n", file_name + index);
            fp = fopen(file_name + index, "wb");
            if (fp == NULL)
            {
                printf("Could not open file to write data\n");
                continue;
            }

            char *temp = strstr(response, "\r\n\r\n");
            int start = temp - response + 4;
            for (int i = start; i < len; i++)
            {
                fwrite(response + i, 1, 1, fp);
            }

            int totalBytesRecieved = len - start;
            int req_bytes = atoi(content_length);

            // Read body
            // printf("About to read body\n");
            struct pollfd fds;
            fds.events = POLLIN;
            fds.revents = 0;
            fds.fd = sock;
            flag = 0;
            while (totalBytesRecieved < req_bytes)
            {
                // POLL
                poll_return = poll(&pfd, 1, 3000);
                if (poll_return == 0)
                {
                    printf("Timed Out: No response by the server\n");
                    flag = 1;
                    break;
                }
                else if (poll_return < 0)
                {
                    perror("Poll failed!\n");
                    exit(0);
                }
                int bytesRecieved = recv(sock, buffer, 50, 0);
                fwrite(buffer, 1, bytesRecieved, fp);
                totalBytesRecieved += bytesRecieved;
            }

            // printf("Body read sucessfully\n");
            if (flag == 1)
            {
                continue;
            }

            fclose(fp);

            // printf("File Closed!\n");

            if (content_cases == 0)
            {
                // html file
                if (fork() == 0)
                {
                    char *args[] = {"firefox", file_name + index, NULL};
                    execvp(args[0], args);
                    perror("execvp failed");
                    exit(EXIT_FAILURE);
                }
            }
            else if (content_cases == 1)
            {
                // pdf file
                if (fork() == 0)
                {
                    char *args[] = {"firefox", file_name + index, NULL};
                    execvp(args[0], args);
                    perror("execvp failed");
                    exit(EXIT_FAILURE);
                }
            }
            else if (content_cases == 2)
            {
                // jpeg file
                if (fork() == 0)
                {
                    char *args[] = {"firefox", file_name + index, NULL};
                    execvp(args[0], args);
                    perror("execvp failed");
                    exit(EXIT_FAILURE);
                }
            }
            else if (content_cases == 3)
            {
                // other file
                if (fork() == 0)
                {
                    char *args[] = {"gedit", file_name + index, NULL};
                    execvp(args[0], args);
                    perror("execvp failed");
                    exit(EXIT_FAILURE);
                }
            }

            // Close connection
            close(sock);
        }

        // Handle put command
        else if (strcmp(token, "PUT") == 0)
        {

            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("Error: Invalid Command\n");
                continue;
            }
            char *url = token;
            token = strtok(NULL, " ");
            if (token == NULL)
            {
                printf("Error: Invalid Command\n");
                continue;
            }
            char *file_to_send = token;
            token = strtok(NULL, " ");
            if (token != NULL)
            {
                printf("Error: Invalid Command\n");
                continue;
            }

            char *ip = get_ip(url);
            char *port = get_port(url);

            // printf("IP:%s\nPort:%s\n", ip, port);

            // // Create socket
            int sock;
            struct sockaddr_in server_addr;
            sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0)
            {
                perror("Could not create socket");
                exit(0);
            }

            server_addr.sin_family = AF_INET;
            inet_aton(ip, &server_addr.sin_addr);
            server_addr.sin_port = htons(atoi(port));

            if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
            {
                perror("Could not connect to the server");
                exit(0);
            }

            printf("Connected to Server....\n");

            // Get path
            char *path = (char *)malloc(strlen(input) * sizeof(char));
            strcpy(path, "PUT /");
            int flag = 0;
            int index = 5;
            for (int i = 0; url[i] != '\0'; i++)
            {
                if (flag > 3)
                {
                    if (url[i] == ':')
                    {
                        break;
                    }
                    path[index] = url[i];
                    index++;
                }
                if (url[i] == '/' || url[i] == ':')
                {
                    flag++;
                }
            }
            path[index] = '\0';
            if (index != 5)
            {
                strncat(path, "/", 2);
            }
            strncat(path, file_to_send, strlen(file_to_send) + 1);
            strncat(path, " HTTP/1.1", 10);

            char accept_lan[20] = "en-US,en;q=0.7";

            time_t now;
            time(&now);
            char date[50];
            strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now));

            int last_dot = -1;
            int len = strlen(file_to_send);
            for (int i = 0; i < len; i++)
            {
                if (file_to_send[i] == '.')
                {
                    last_dot = i;
                }
            }
            char content_type[20] = "";
            if (file_to_send[last_dot + 1] == 'h' && file_to_send[last_dot + 2] == 't' && file_to_send[last_dot + 3] == 'm' && file_to_send[last_dot + 4] == 'l' && (file_to_send[last_dot + 5] == '\0' || file_to_send[last_dot + 5] == ':'))
            {
                strcpy(content_type, "text/html");
            }
            else if (file_to_send[last_dot + 1] == 'p' && file_to_send[last_dot + 2] == 'd' && file_to_send[last_dot + 3] == 'f' && (file_to_send[last_dot + 4] == '\0' || file_to_send[last_dot + 4] == ':'))
            {
                strcpy(content_type, "application/pdf");
            }
            else if (file_to_send[last_dot + 1] == 'j' && file_to_send[last_dot + 2] == 'p' && file_to_send[last_dot + 3] == 'g' && (file_to_send[last_dot + 4] == '\0' || file_to_send[last_dot + 4] == ':'))
            {
                strcpy(content_type, "image/jpeg");
            }
            else
            {
                strcpy(content_type, "text/*");
            }

            FILE *fp;
            fp = fopen(file_to_send, "rb");
            if (fp == NULL)
            {
                printf("Error opening file\n");
                continue;
            }

            fseek(fp, 0, SEEK_END);
            long long int file_size = ftell(fp);
            fclose(fp);
            fp = fopen(file_to_send, "rb");

            char content_length[20];
            sprintf(content_length, "%lld", file_size);

            // Create http request
            char header[5000] = "";
            char delimieter[5] = "\r\n";
            strncat(header, path, strlen(path) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Host: ", 7);
            strncat(header, ip, strlen(ip) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Connection: close", 20);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Date: ", 7);
            strncat(header, date, strlen(date) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Accept-Language: ", 20);
            strncat(header, accept_lan, strlen(accept_lan) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Content-Language: en-US", 25);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Content-Length: ", 20);
            strncat(header, content_length, strlen(content_length) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, "Content-Type: ", 20);
            strncat(header, content_type, strlen(content_type) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            strncat(header, delimieter, strlen(delimieter) + 1);
            printf("Sending:\n%s", header);

            if (send(sock, header, strlen(header), 0) <= 0)
            {
                perror("Server Down!");
                exit(0);
            }

            // Send body

            char send_buff[MAXLEN];
            int rem = file_size;
            int temp;
            while (rem > 0)
            {

                if (rem >= MAXLEN)
                {
                    temp = fread(send_buff, 1, MAXLEN, fp);
                    if (send(sock, send_buff, MAXLEN, 0) < 0)
                    {
                        break;
                    }
                    rem -= MAXLEN;
                }
                else
                {
                    temp = fread(send_buff, 1, rem, fp);
                    if (send(sock, send_buff, MAXLEN, 0) < 0)
                    {
                        break;
                    }
                    rem = 0;
                }
            }
            fclose(fp);
            // printf("Body Sent\n");
            // Read http response
            char buffer[MAXLEN];

            struct pollfd pfd;
            pfd.events = POLLIN;
            pfd.revents = 0;
            pfd.fd = sock;

            // Wait for maximum 3 seconds
            len = -1;
            int poll_return = poll(&pfd, 1, 3000);
            char *response = NULL;
            if (poll_return > 0)
            {
                response = recieve_data(sock, buffer, &len);
                // printf("REcieved Response: \n%s", response);
                //
                //
                printf("Recieved Request:\n");
                char *end = strstr(response, "\r\n\r\n");
                for (int i = 0; i < (end - response) + 4; i++)
                {
                    printf("%c", response[i]);
                }
            }
            else if (poll_return < 0)
            {
                perror("Poll failed!\n");
                exit(0);
            }
            else
            {
                printf("Timed Out: No response by the server\n");
                continue;
            }

            // printf("Reached here\n");
            char *response_dup = strdup(response);

            // Check for errors and print them (Check if any file is sent or not using content length)
            // Get Status
            token = strtok(response_dup, "\r\n");
            char *code = strtok(token, " ");
            code = strtok(NULL, " ");
            if (strcmp(code, "400") == 0)
            {
                printf("Bad Request\n");
            }
            else if (strcmp(code, "403") == 0)
            {
                printf("Forbidden\n");
            }
            else if (strcmp(code, "404") == 0)
            {
                printf("Not Found\n");
            }
            else if (strcmp(code, "200") != 0)
            {
                printf("Unknown Error Code: %s\n", code);
            }

            // Get Content Length and Content Type

            char cont_type[20] = "#";
            char cont_length[100] = "#";
            char *response_dup2 = strdup(response);
            token = strtok(response_dup2, "\r\n");
            // Walk through other tokens
            while (token != NULL)
            {
                char *temp1 = strstr(token, "Content-Length:");
                if (temp1)
                {
                    strcpy(cont_length, temp1 + 16);
                }
                char *temp2 = strstr(token, "Content-Type:");
                if (temp2)
                {
                    strcpy(cont_type, temp2 + 14);
                }
                token = strtok(NULL, "\r\n");
            }

            if (strcmp(cont_length, "#") != 0 && strcmp(cont_type, "text/html") == 0)
            {

                char err_file[20] = "err_file.html";
                fp = fopen(err_file, "wb");
                if (fp == NULL)
                {
                    printf("Could not open file to write data\n");
                    continue;
                }

                char *temp = strstr(response, "\r\n\r\n");
                int start = temp - response + 4;
                for (int i = start; i < len; i++)
                {
                    fwrite(response + i, 1, 1, fp);
                }

                int totalBytesRecieved = len - start;
                int req_bytes = atoi(cont_length);

                while (totalBytesRecieved < req_bytes)
                {
                    int bytesRecieved = recv(sock, buffer, 50, 0);
                    fwrite(buffer, 1, bytesRecieved, fp);
                    totalBytesRecieved += bytesRecieved;
                }

                // printf("Body read sucessfully\n");

                fclose(fp);

                // html file
                if (fork() == 0)
                {
                    char *args[] = {"firefox", err_file, NULL};
                    execvp(args[0], args);
                    perror("execvp failed");
                    exit(EXIT_FAILURE);
                }
            }
        }

        else
        {
            printf("Error: Invalid Command\n");
            continue;
        }
    }

    return 0;
}