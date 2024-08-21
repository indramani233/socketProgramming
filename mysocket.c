#include "mysocket.h"

pthread_t R, S;
pthread_mutex_t send_lock;
pthread_mutex_t receive_lock;
int newsockfd = -1;
Table *Send_Message;
Table *Received_Message;

void *func_R(void *param)
{
    while (newsockfd == -1)
    {
        // wait till newsockfd is set to a value in accept/connect
    }
    while (1)
    {
        // getting the size of the message
        char temp_size[5];
        temp_size[4] = '\0';
        int len = 0;
        while (len != 4)
        {
            int temp = recv(newsockfd, temp_size + len, 4 - len, 0);
            len += temp;
        }
        int sz = atoi(temp_size);
        // receiving the message in a loop
        void *buffer = malloc(sz);
        len = 0;
        while (len != sz)
        {
            int temp = recv(newsockfd, buffer + len, sz - len, 0);
            len += temp;
        }

        // waiting if table is full
        while (1)
        {
            pthread_mutex_lock(&receive_lock);
            if (Received_Message->counter < 10)
            {
                break;
            }
            pthread_mutex_unlock(&receive_lock);
            sleep(1);
        }
        // storing message in the table
        Received_Message->table[Received_Message->in].msg_size = sz;
        strncpy(Received_Message->table[Received_Message->in].msg, buffer, sz);
        Received_Message->in = (Received_Message->in + 1) % 10;
        Received_Message->counter++;
        pthread_mutex_unlock(&receive_lock);

        free(buffer);
    }
}

void *func_S(void *param)
{
    while (1)
    {
        // Wait till number of entries in table is greater than 0
        pthread_mutex_lock(&send_lock);
        while (Send_Message->counter == 0)
        {
            pthread_mutex_unlock(&send_lock);
            sleep(1);
            pthread_mutex_lock(&send_lock);
        }

        void *buf = malloc(5000);
        strcpy(buf, Send_Message->table[Send_Message->out].msg);
        int len = Send_Message->table[Send_Message->out].msg_size;
        Send_Message->out = (Send_Message->out + 1) % 10;
        Send_Message->counter = (Send_Message->counter - 1);

        pthread_mutex_unlock(&send_lock);

        // Send Header bytes
        int temp = len;
        char header[5];
        int i = 4;
        while (i--)
        {
            header[i] = '0' + temp % 10;
            temp /= 10;
        }
        header[4] = '\0';
        int num_bytes;
        if ((num_bytes = send(newsockfd, header, 4, 0)) < 0)
        {
            perror("Could not send header!\n");
            exit(0);
        }
        // Send Message
        int loop_num = 0;
        int loop_data = 0;
        while (len > 0)
        {
            if (len > 1000)
            {
                loop_data = 1000;
            }
            else
            {
                loop_data = len;
            }

            if ((num_bytes = send(newsockfd, buf + 1000 * loop_num, loop_data, 0)) < 0)
            {
                perror("Could not send message!\n");
                exit(0);
            }
            loop_num++;
            len -= loop_data;
        }
        free(buf);
    }
}

int my_socket(int domain, int type, int protocol)
{
    // error if socket type is not SOCK_MyTCP
    if (type != SOCK_MyTCP)
    {
        return -1;
    }
    int sockfd = socket(domain, SOCK_STREAM, protocol);

    // TODO define 2 tables
    Send_Message = (Table *)malloc(sizeof(Table));
    Received_Message = (Table *)malloc(sizeof(Table));

    // Initialize table variables
    Send_Message->counter = 0;
    Send_Message->in = 0;
    Send_Message->out = 0;
    Received_Message->counter = 0;
    Received_Message->in = 0;
    Received_Message->out = 0;

    // mallocing 5000 bytes for every message
    for (int i = 0; i < 10; i++)
    {
        Send_Message->table[i].msg = malloc(5000);
        Received_Message->table[i].msg = malloc(5000);
    }
    // creating threads R and S
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    // TODO write function for threads
    pthread_create(&R, &attr, func_R, NULL);
    pthread_create(&S, &attr, func_S, NULL);

    return sockfd;
}

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return bind(sockfd, addr, addrlen);
}

int my_listen(int sockfd, int backlog)
{
    return listen(sockfd, backlog);
}

int my_accept(int sockfd, struct sockaddr *restrict addr, socklen_t *restrict addrlen)
{
    newsockfd = accept(sockfd, addr, addrlen);
    return newsockfd;
}

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    connect(sockfd, addr, addrlen);
    newsockfd = sockfd;
    return newsockfd;
}

ssize_t my_send(int sockfd, const void *buf, size_t len, int flags)
{

    // Check for valid length
    if (len < 0 || len > 5000)
    {
        return -1;
    }

    // Wait till number of entries in table is less than 10
    pthread_mutex_lock(&send_lock);
    while (Send_Message->counter == 10)
    {
        pthread_mutex_unlock(&send_lock);
        sleep(1);
        pthread_mutex_lock(&send_lock);
    }

    // Put message in send table
    strcpy(Send_Message->table[Send_Message->in].msg, buf);
    Send_Message->table[Send_Message->in].msg_size = len;
    Send_Message->in = (Send_Message->in + 1) % 10;
    Send_Message->counter = (Send_Message->counter + 1);

    pthread_mutex_unlock(&send_lock);

    return len;
}

ssize_t my_recv(int sockfd, void *buf, size_t len, int flags)
{
    while (1)
    {
        pthread_mutex_lock(&receive_lock);
        if (Received_Message->counter > 0)
        {
            break;
        }
        pthread_mutex_unlock(&receive_lock);
        sleep(1);
    }
    int sz = Received_Message->table[Received_Message->out].msg_size;
    int temp = sz;
    if (len < sz)
        temp = len;

    strncpy(buf, Received_Message->table[Received_Message->out].msg, temp);
    Received_Message->out = (Received_Message->out + 1) % 10;
    Received_Message->counter--;
    pthread_mutex_unlock(&receive_lock);
    return temp;
}

int my_close(int fd)
{
    sleep(5);
    pthread_cancel(R);
    pthread_cancel(S);
    for (int i = 0; i < 10; i++)
    {
        free(Send_Message->table[i].msg);
        free(Received_Message->table[i].msg);
    }
    free(Send_Message);
    free(Received_Message);
    return close(fd);
}