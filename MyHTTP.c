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
#include <sys/stat.h>
#include <fcntl.h>

#define MAXLEN 50

char bad_request[200] = "<html><head></head><body><p><font size=\"5\">Error 400:<br> Bad Request</font></p></body></html>";
char forbidden[200] = "<html><head></head><body><p><font size=\"5\">Error 403:<br> Forbidden</font></p></body></html>";
char not_found[200] = "<html><head></head><body><p><font size=\"5\">Error 404:<br> Not Found</font></p></body></html>";
char unknown_error[200] = "<html><head></head><body><p><font size=\"5\">Unknown Error</font></p></body></html>";

int file_details(char *file)
{
    int temp = 0;
    if (access(file, F_OK) != 0)
    {
        return temp;
    }
    else
    {
        temp += 1;
    }

    if (access(file, R_OK) == 0)
    {
        temp += 10;
    }
    if (access(file, W_OK) == 0)
    {
        temp += 100;
    }

    return temp;
}

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

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Error: Number of arguments mismatched\nUsage: ./<filename> <Port Number>\n");
        return 0;
    }
    int sock, newSock;                           // Socket Descriptors
    struct sockaddr_in client_addr, server_addr; // Structure to store address
    char buffer[MAXLEN];

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0)
    {
        perror("Could not create socket\n");
        exit(0);
    }

    // Filling the server address details
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(atoi(argv[1]));

    // Binding server address to socket
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Could not bind local address\n");
        exit(0);
    }

    printf("Server is up and running....\n");

    listen(sock, 5);

    while (1)
    {
        int len = sizeof(client_addr);
        newSock = accept(sock, (struct sockaddr *)&client_addr, &len);
        if (newSock < 0)
        {
            perror("Could not accept!\n");
            exit(0);
        }

        // Create Child Process
        if (fork() == 0)
        {
            close(sock);

            FILE *logfile;
            logfile = fopen("AccessLog.txt", "a+");
            if (logfile == NULL)
                return -1;

            // Recieve HTTP Request
            int len = -1;
            char *response = recieve_data(newSock, buffer, &len);
            // printf("%s", response);
            //
            printf("Recieved Request:\n");
            char *end = strstr(response, "\r\n\r\n");
            for (int i = 0; i < (end - response) + 4; i++)
            {
                printf("%c", response[i]);
            }
            // Parse HTTP Request
            char *response_dup = strdup(response);

            char clntName[INET_ADDRSTRLEN]; // String to contain client address
            inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, clntName, sizeof(clntName));

            char *token;
            // get the first token
            token = strtok(response_dup, "\r\n");
            char *token1_dup = strdup(token);

            // get file path
            char *path = (char *)malloc(strlen(response) * sizeof(char));
            int index = 0;
            for (int i = 4; token[i] != ' '; i++)
            {
                path[index] = token[i];
                index++;
            }
            path[index] = '\0';

            // Walk through other tokens
            char date[50] = "#";
            char host_ip[20] = "#";
            char if_modified_since[50] = "#";
            char accept[20] = "#";
            char body_len[20] = "#";

            while (token != NULL)
            {
                char *t1 = strstr(token, "Date:");

                if (t1)
                {
                    strcpy(date, t1 + 6);
                }
                char *t2 = strstr(token, "Host:");

                if (t2)
                {
                    strcpy(host_ip, t2 + 6);
                }
                char *t3 = strstr(token, "If-Modified-Since:");
                if (t3)
                {
                    strcpy(if_modified_since, t3 + 19);
                }
                char *t4 = strstr(token, "Accept:");
                if (t4)
                {
                    strcpy(accept, t4 + 8);
                }
                char *t5 = strstr(token, "Content-Length:");
                if (t5)
                {
                    strcpy(body_len, t5 + 16);
                }
                token = strtok(NULL, "\r\n");
            }

            time_t now, now_plus_3;
            time(&now);
            now_plus_3 = now + (3 * 24 * 60 * 60);
            char expires[50];
            strftime(expires, sizeof(expires), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&now_plus_3));

            if (strcmp(host_ip, "#") == 0 || strcmp(date, "#") == 0)
            {
                // Bad request 400
                char header[5000] = "";
                char delimieter[5] = "\r\n";
                strncat(header, "HTTP/1.1 400 Bad Request", 40);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Expires: ", 10);
                strncat(header, expires, strlen(expires) + 1);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Cache-Control: no-store", 30);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Content-Language: en-US", 30);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Content-Length: ", 20);

                char content_length[20];
                sprintf(content_length, "%ld", strlen(bad_request));

                strncat(header, content_length, strlen(content_length) + 1);

                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Content-Type: text/html", 30);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, delimieter, strlen(delimieter) + 1);

                if (send(newSock, header, strlen(header), 0) < 0)
                {
                    perror("Error while sending");
                    exit(0);
                }
                if (send(newSock, bad_request, strlen(bad_request), 0) < 0)
                {
                    perror("Error while sending");
                    exit(0);
                }

                exit(0);
            }
            char ddmmyy[100];
            struct tm tm;
            strptime(date, "%a, %d %b %Y %T GMT", &tm);
            strftime(ddmmyy, 100, "%d%m%y", &tm);

            char hhmmss[100];
            strftime(hhmmss, 100, "%H%M%S", &tm);

            char *temp1 = strstr(token1_dup, "GET");
            char *temp2 = strstr(token1_dup, "PUT");

            if (temp1 == token1_dup)
            {
                // GET Request
                fprintf(logfile, "%s:%s:%s:%d:GET:html://%s%s\n", ddmmyy, hhmmss, clntName, ntohs(client_addr.sin_port), host_ip, path);
                fclose(logfile);

                // Send HTTP response (After handling error codes)

                int permissions = file_details(path + 1);
                if (permissions % 10 == 0 || strlen(path) <= 5)
                {
                    // Not Found 404
                    char header[5000] = "";
                    char delimieter[5] = "\r\n";
                    strncat(header, "HTTP/1.1 404 Not Found", 40);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Expires: ", 10);
                    strncat(header, expires, strlen(expires) + 1);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Cache-Control: no-store", 30);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Content-Language: en-US", 30);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Content-Length: ", 20);

                    char content_length[20];
                    sprintf(content_length, "%ld", strlen(not_found));

                    strncat(header, content_length, strlen(content_length) + 1);

                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Content-Type: text/html", 30);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    printf("Response Sent:\n");
                    char *end = strstr(header, "\r\n\r\n");
                    for (int i = 0; i < (end - header) + 4; i++)
                    {
                        printf("%c", header[i]);
                    }
                    if (send(newSock, header, strlen(header), 0) < 0)
                    {
                        perror("Error while sending");
                        exit(0);
                    }
                    if (send(newSock, not_found, strlen(not_found), 0) < 0)
                    {
                        perror("Error while sending");
                        exit(0);
                    }

                    exit(0);
                }
                permissions /= 10;
                if (permissions % 10 == 0)
                {
                    // Forbidden 403
                    char header[5000] = "";
                    char delimieter[5] = "\r\n";
                    strncat(header, "HTTP/1.1 403 Forbidden", 40);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Expires: ", 10);
                    strncat(header, expires, strlen(expires) + 1);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Cache-Control: no-store", 30);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Content-Language: en-US", 30);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Content-Length: ", 20);

                    char content_length[20];
                    sprintf(content_length, "%ld", strlen(forbidden));

                    strncat(header, content_length, strlen(content_length) + 1);

                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Content-Type: text/html", 30);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    printf("Response Sent:\n");
                    char *end = strstr(header, "\r\n\r\n");
                    for (int i = 0; i < (end - header) + 4; i++)
                    {
                        printf("%c", header[i]);
                    }
                    if (send(newSock, header, strlen(header), 0) < 0)
                    {
                        perror("Error while sending");
                        exit(0);
                    }
                    if (send(newSock, forbidden, strlen(forbidden), 0) < 0)
                    {
                        perror("Error while sending");
                        exit(0);
                    }
                    exit(0);
                }
                else
                {
                    // Send file 200
                    struct stat sb;
                    if (stat(path + 1, &sb) == -1)
                    {
                        perror("stat");
                        return 1;
                    }

                    struct tm tm;
                    memset(&tm, 0, sizeof(struct tm));
                    time_t if_modified_since_time;
                    if (strcmp(if_modified_since, "#") == 0)
                    {
                        if_modified_since_time = 0;
                    }
                    else
                    {
                        strptime(if_modified_since, "%a, %d %b %Y %T %Z", &tm);
                        if_modified_since_time = mktime(&tm);
                    }

                    if (sb.st_mtime > if_modified_since_time)
                    {
                        // the file was modified after the date specified in the "If-Modified-Since" header
                        // Send file

                        // Get file type
                        char content_type[20] = "";
                        if (path[strlen(path) - 5] == '.' && path[strlen(path) - 4] == 'h' && path[strlen(path) - 3] == 't' && path[strlen(path) - 2] == 'm' && path[strlen(path) - 1] == 'l')
                        {
                            strcpy(content_type, "text/html");
                        }
                        else if (path[strlen(path) - 4] == '.' && path[strlen(path) - 3] == 'p' && path[strlen(path) - 2] == 'd' && path[strlen(path) - 1] == 'f')
                        {
                            strcpy(content_type, "application/pdf");
                        }
                        else if (path[strlen(path) - 4] == '.' && path[strlen(path) - 3] == 'j' && path[strlen(path) - 2] == 'p' && path[strlen(path) - 1] == 'g')
                        {
                            strcpy(content_type, "image/jpeg");
                        }
                        else
                        {
                            strcpy(content_type, "text/*");
                        }

                        FILE *fp;
                        fp = fopen(path + 1, "rb");
                        if (fp == NULL)
                        {
                            printf("Error opening file\n");
                            continue;
                        }

                        fseek(fp, 0, SEEK_END);
                        long long int file_size = ftell(fp);
                        fclose(fp);
                        fp = fopen(path + 1, "rb");

                        char content_length[20];
                        sprintf(content_length, "%lld", file_size);
                        char last_modified[100];
                        strcpy(last_modified, ctime(&sb.st_mtime));
                        char header[5000] = "";
                        char delimieter[5] = "\r\n";
                        strncat(header, "HTTP/1.1 200 OK", 25);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Expires: ", 10);
                        strncat(header, expires, strlen(expires) + 1);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Cache-Control: no-store", 30);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Content-Language: en-US", 30);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Content-Length: ", 20);

                        strncat(header, content_length, strlen(content_length) + 1);

                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Content-Type: ", 30);
                        strncat(header, content_type, strlen(content_type) + 1);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Last-Modified: ", 30);
                        strncat(header, last_modified, strlen(last_modified) + 1);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, delimieter, strlen(delimieter) + 1);

                        if (send(newSock, header, strlen(header), 0) < 0)
                        {
                            perror("Error while sending");
                            exit(0);
                        }

                        printf("Response Sent:\n");
                        char *end = strstr(header, "\r\n\r\n");
                        for (int i = 0; i < (end - header) + 4; i++)
                        {
                            printf("%c", header[i]);
                        }
                        // printf("Headers sent\n");
                        char send_buff[MAXLEN];
                        size_t rem = file_size;
                        int temp;
                        while (rem > 0)
                        {
                            if (rem >= MAXLEN)
                            {
                                temp = fread(send_buff, 1, MAXLEN, fp);
                                send(newSock, send_buff, MAXLEN, 0);
                                rem -= MAXLEN;
                            }
                            else
                            {
                                temp = fread(send_buff, 1, rem, fp);
                                send(newSock, send_buff, rem, 0);
                                rem = 0;
                            }
                        }
                        // printf("File sent, closing child process\n");
                        exit(0);
                    }
                    else
                    {
                        // the file was not modified after the date specified in the "If-Modified-Since" header
                        // Send Unknown error (304)
                        char header[5000] = "";
                        char delimieter[5] = "\r\n";
                        strncat(header, "HTTP/1.1 304 Not Modified", 40);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Expires: ", 10);
                        strncat(header, expires, strlen(expires) + 1);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Cache-Control: no-store", 30);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Content-Language: en-US", 30);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Content-Length: ", 20);

                        char content_length[20];
                        sprintf(content_length, "%ld", strlen(unknown_error));

                        strncat(header, content_length, strlen(content_length) + 1);

                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, "Content-Type: text/html", 30);
                        strncat(header, delimieter, strlen(delimieter) + 1);
                        strncat(header, delimieter, strlen(delimieter) + 1);

                        printf("Response Sent:\n");
                        char *end = strstr(header, "\r\n\r\n");
                        for (int i = 0; i < (end - header) + 4; i++)
                        {
                            printf("%c", header[i]);
                        }
                        if (send(newSock, header, strlen(header), 0) < 0)
                        {
                            perror("Error while sending");
                            exit(0);
                        }
                        if (send(newSock, unknown_error, strlen(unknown_error), 0) < 0)
                        {
                            perror("Error while sending");
                            exit(0);
                        }
                        exit(0);
                    }
                }
            }
            else if (temp2 == token1_dup)
            {
                // PUT Request
                fprintf(logfile, "%s:%s:%s:%d:PUT:html://%s%s\n", ddmmyy, hhmmss, clntName, ntohs(client_addr.sin_port), host_ip, path);
                fclose(logfile);

                // printf("Request: \n%s", response);
                //
                // char *end = strstr(response, "\r\n\r\n");
                // for (int i = 0; i < (end - response) + 4; i++)
                // {
                //     printf("%c", response[i]);
                // }
                // PUT File
                int permissions = file_details(path + 1);
                FILE *fp;
                fp = fopen(path + 1, "wb");
                if (fp == NULL)
                {
                    // forbidden 403
                    char header[5000] = "";
                    char delimieter[5] = "\r\n";
                    strncat(header, "HTTP/1.1 403 Forbidden", 40);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Expires: ", 10);
                    strncat(header, expires, strlen(expires) + 1);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Cache-Control: no-store", 30);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Content-Language: en-US", 30);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Content-Length: ", 20);

                    char content_length[20];
                    sprintf(content_length, "%ld", strlen(forbidden));

                    strncat(header, content_length, strlen(content_length) + 1);

                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, "Content-Type: text/html", 30);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    strncat(header, delimieter, strlen(delimieter) + 1);
                    printf("Response Sent:\n");
                    char *end = strstr(header, "\r\n\r\n");
                    for (int i = 0; i < (end - header) + 4; i++)
                    {
                        printf("%c", header[i]);
                    }
                    if (send(newSock, header, strlen(header), 0) < 0)
                    {
                        perror("Error while sending");
                        exit(0);
                    }
                    if (send(newSock, forbidden, strlen(forbidden), 0) < 0)
                    {
                        perror("Error while sending");
                        exit(0);
                    }
                    close(newSock);
                    exit(0);
                }

                char *temp = strstr(response, "\r\n\r\n");
                int start = temp - response + 4;
                for (int i = start; i < len; i++)
                {
                    fwrite(response + i, 1, 1, fp);
                }
                int totalBytesRecieved = len - start;
                size_t req_bytes = atoi(body_len);

                while (totalBytesRecieved < req_bytes)
                {
                    int bytesRecieved = recv(newSock, buffer, 50, 0);
                    fwrite(buffer, 1, bytesRecieved, fp);
                    totalBytesRecieved += bytesRecieved;
                }
                fclose(fp);

                // printf("Body read sucessfully\n");

                // Send HTTP response (After handling error codes)
                char header[5000] = "";
                char delimieter[5] = "\r\n";
                strncat(header, "HTTP/1.1 200 OK", 40);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Expires: ", 10);
                strncat(header, expires, strlen(expires) + 1);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Cache-Control: no-store", 30);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Content-Language: en-US", 30);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, delimieter, strlen(delimieter) + 1);
                printf("Response Sent:\n");
                char *end = strstr(header, "\r\n\r\n");
                for (int i = 0; i < (end - header) + 4; i++)
                {
                    printf("%c", header[i]);
                }
                if (send(newSock, header, strlen(header), 0) < 0)
                {
                    perror("Error while sending");
                    exit(0);
                }

                exit(0);
            }
            else
            {
                // BAD REQUEST
                char header[5000] = "";
                char delimieter[5] = "\r\n";
                strncat(header, "HTTP/1.1 400 Bad Request", 40);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Expires: ", 10);
                strncat(header, expires, strlen(expires) + 1);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Cache-Control: no-store", 30);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Content-Language: en-US", 30);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Content-Length: ", 20);

                char content_length[20];
                sprintf(content_length, "%ld", strlen(bad_request));

                strncat(header, content_length, strlen(content_length) + 1);

                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, "Content-Type: text/html", 30);
                strncat(header, delimieter, strlen(delimieter) + 1);
                strncat(header, delimieter, strlen(delimieter) + 1);

                printf("Response Sent:\n");
                char *end = strstr(header, "\r\n\r\n");
                for (int i = 0; i < (end - header) + 4; i++)
                {
                    printf("%c", header[i]);
                }
                if (send(newSock, header, strlen(header), 0) < 0)
                {
                    perror("Error while sending");
                    exit(0);
                }
                if (send(newSock, bad_request, strlen(bad_request), 0) < 0)
                {
                    perror("Error while sending");
                    exit(0);
                }
                exit(0);
            }

            close(newSock);
            exit(0);
        }

        close(newSock);
    }
    return 0;
}