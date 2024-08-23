/*
    Name: Anuj Kakde
    Roll No: 20CS30005

    Name: Sarthak Nikumbh
    Roll No: 20CS30035
*/
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/time.h>

#define PACKET_SIZE 64 // Size of ICMP packet data
#define MAX_HOPS 30    // Maximum number of hops for traceroute
#define MAX_PROBES 5   // Number of probes per link
#define TIMEOUT 1      // Timeout in seconds for each probe
#define PACKET_SIZE 64 // Max packet size
#define ICMP_ECHO_REQUEST 8
#define ICMP_ECHO_REPLY 0
#define MAX_WAIT_TIME 1
#define MAX_NO_PACKETS 20

char sendbuf[PACKET_SIZE];
char recvbuf[PACKET_SIZE];
int sockfd;
int eps = 0.357;
struct sockaddr_in dest_addr;
struct sockaddr_in src_addr;
struct sockaddr_in temp_addr;

void print_icmp_packet(struct icmphdr *icmp_header);
void print_ip_header(struct iphdr *ip_header);
unsigned short checksum(unsigned short *buf, int nwords);
void handle_packet(unsigned char *buffer, int size);
void send_ping(int ttl, int size_in_bytes, struct timeval *start, int flag);
int recv_ping(int ttl, struct timeval *end);

// Function to calculate checksum of ICMP packet
unsigned short checksum(unsigned short *buf, int nwords)
{
    unsigned long sum;
    for (sum = 0; nwords > 0; nwords--)
        sum += *buf++;
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    return ~sum;
}

// Function to print headers of different type of packets
void handle_packet(unsigned char *buffer, int size)
{
    struct iphdr *ip_header = (struct iphdr *)buffer;
    unsigned short ip_header_len = ip_header->ihl * 4;
    print_ip_header((struct iphdr *)ip_header);
    switch (ip_header->protocol)
    {
    case IPPROTO_UDP:
        printf("Received UDP packet\n");

        struct udphdr *udp_header = (struct udphdr *)(buffer + ip_header_len);
        printf("Source port: %d\n", ntohs(udp_header->source));
        printf("Destination port: %d\n", ntohs(udp_header->dest));
        printf("Length: %d\n", ntohs(udp_header->len));
        printf("Checksum: %d\n", ntohs(udp_header->check));

        // Print body if any
        int udp_data_size = size - ip_header_len - sizeof(struct udphdr);
        unsigned char *udp_data = buffer + ip_header_len + sizeof(struct udphdr);
        printf("Data: ");
        for (int i = 0; i < udp_data_size; i++)
        {
            printf("%02x ", udp_data[i]);
        }
        printf("\n");
        break;
    case IPPROTO_TCP:
        printf("Received TCP packet\n");

        struct tcphdr *tcp_header = (struct tcphdr *)(buffer + ip_header_len);
        printf("Source port: %d\n", ntohs(tcp_header->source));
        printf("Destination port: %d\n", ntohs(tcp_header->dest));
        printf("Sequence number: %u\n", ntohl(tcp_header->seq));
        printf("Ack number: %u\n", ntohl(tcp_header->ack_seq));
        printf("Data offset: %d\n", tcp_header->doff);
        printf("Flags:");
        if (tcp_header->urg)
            printf(" URG");
        if (tcp_header->ack)
            printf(" ACK");
        if (tcp_header->psh)
            printf(" PSH");
        if (tcp_header->rst)
            printf(" RST");
        if (tcp_header->syn)
            printf(" SYN");
        if (tcp_header->fin)
            printf(" FIN");
        printf("\n");
        printf("Window size: %d\n", ntohs(tcp_header->window));
        printf("Checksum: %d\n", ntohs(tcp_header->check));
        printf("Urgent pointer: %d\n", ntohs(tcp_header->urg_ptr));

        // Print body if any
        int tcp_data_size = size - ip_header_len - tcp_header->doff * 4;
        unsigned char *tcp_data = buffer + ip_header_len + tcp_header->doff * 4;
        printf("Data: ");
        for (int i = 0; i < tcp_data_size; i++)
        {
            printf("%02x ", tcp_data[i]);
        }
        printf("\n");
        break;
    case IPPROTO_ICMP:

        print_icmp_packet((struct icmphdr *)(buffer + (ip_header->ihl << 2)));
        printf("\n");
        break;
    default:
        printf("Received packet of unknown protocol\n");
        break;
    }
}

// Function to send packet
void send_ping(int ttl, int size_in_bytes, struct timeval *start, int flag)
{
    int packsize;
    struct icmp *icmp;
    icmp = (struct icmp *)sendbuf;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_id = getpid();
    icmp->icmp_seq = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_cksum = checksum((unsigned short *)icmp, size_in_bytes + 8);
    setsockopt(sockfd, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl));
    packsize = size_in_bytes + 8;
    struct in_addr adr;
    adr.s_addr = INADDR_ANY;
    printf("Sent Packet Headers-\n\n");
    printf("IP Header:\n");
    printf("    Version: 4\n");
    printf("    Header Length: %d\n", 20);
    printf("    TOS: %d\n", 0);
    printf("    TTL: %d\n", ttl);
    printf("    Protocol: %d\n", IPPROTO_ICMP);
    printf("    Source IP Address: %s\n", inet_ntoa(adr));
    if (flag != 1)
        printf("    Destination IP Address: %s\n", inet_ntoa(dest_addr.sin_addr));
    else
        printf("    Destination IP Address: %s\n", inet_ntoa(temp_addr.sin_addr));
    print_icmp_packet((struct icmphdr *)icmp);

    // Start timer
    gettimeofday(start, NULL);
    if (flag != 1)
    {
        if (sendto(sockfd, sendbuf, packsize, 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) == -1)
        {
            perror("sendto error");
            exit(1);
        }
    }
    else
    {
        if (sendto(sockfd, sendbuf, packsize, 0, (struct sockaddr *)&temp_addr, sizeof(temp_addr)) == -1)
        {
            perror("sendto error");
            exit(1);
        }
    }
}

// Function to recieve packet
int recv_ping(int ttl, struct timeval *end)
{
    int n, fromlen;
    fd_set rfds;
    struct timeval tv;
    struct ip *ip;
    struct icmp *icmp;
    fromlen = sizeof(src_addr);
    FD_ZERO(&rfds);
    FD_SET(sockfd, &rfds);
    tv.tv_sec = MAX_WAIT_TIME;
    tv.tv_usec = 0;
    n = select(sockfd + 1, &rfds, NULL, NULL, &tv);
    if (n == -1)
    {
        perror("select error");
        exit(1);
    }
    if (n == 0)
    {
        printf("\nTimed out\n\n");
        return -1;
    }

    if ((n = recvfrom(sockfd, recvbuf, sizeof(recvbuf), 0, (struct sockaddr *)&src_addr, &fromlen)) == -1)
    {
        perror("recvfrom error");
        exit(1);
    }

    // End timer
    gettimeofday(end, NULL);

    ip = (struct ip *)recvbuf;
    icmp = (struct icmp *)(recvbuf + (ip->ip_hl << 2));
    printf("\nRecieved Headers-\n");
    handle_packet(recvbuf, n);
    // if (icmp->icmp_type == ICMP_ECHO_REPLY)
    // {
    //     printf("Received ICMP echo reply from %s with TTL=%d\n", inet_ntoa(src_addr.sin_addr), ttl);
    // }
    // else
    // {
    //     printf("Received ICMP packet from %s with type=%d\n", inet_ntoa(src_addr.sin_addr), icmp->icmp_type);
    // }
    return 0;
}

// Function to print ICMP packet header fields
void print_icmp_packet(struct icmphdr *icmp_header)
{
    printf("ICMP Header:\n");
    printf("    Type: %d\n", icmp_header->type);
    printf("    Code: %d\n", icmp_header->code);
    printf("    Checksum: %d\n", icmp_header->checksum);
    printf("    Identifier: %d\n", icmp_header->un.echo.id);
    printf("    Sequence Number: %d\n", icmp_header->un.echo.sequence);
}

// Function to print IP header fields
void print_ip_header(struct iphdr *ip_header)
{
    struct sockaddr_in source, dest;
    memset(&source, 0, sizeof(source));
    memset(&dest, 0, sizeof(dest));
    source.sin_addr.s_addr = ip_header->saddr;
    dest.sin_addr.s_addr = ip_header->daddr;

    printf("IP Header:\n");
    printf("    Version: %d\n", ip_header->version);
    printf("    Header Length: %d\n", ip_header->ihl * 4);
    printf("    TOS: %d\n", ip_header->tos);
    printf("    Total Length: %d\n", ntohs(ip_header->tot_len));
    printf("    Identification: %d\n", ntohs(ip_header->id));
    printf("    Flags: %d\n", (ntohs(ip_header->frag_off) & 0xE000) >> 13);
    printf("    Fragment Offset: %d\n", ntohs(ip_header->frag_off) & 0x1FFF);
    printf("    TTL: %d\n", ip_header->ttl);
    printf("    Protocol: %d\n", ip_header->protocol);
    printf("    Source IP Address: %s\n", inet_ntoa(source.sin_addr));
    printf("    Destination IP Address: %s\n", inet_ntoa(dest.sin_addr));
}

int main(int argc, char *argv[])
{
    char *site_address;
    int probes_per_link, time_difference;

    // Parse command-line arguments
    if (argc != 4)
    {
        printf("Usage: ./pingnetinfo.c <site_address> <probes_per_link> <time_difference>\n");
        exit(1);
    }
    site_address = argv[1];
    probes_per_link = atoi(argv[2]);
    time_difference = atof(argv[3]);

    // Get IP address of site using gethostbyname()
    struct hostent *host;
    struct in_addr **addr_list;
    if ((host = gethostbyname(site_address)) == NULL)
    {
        herror("Could not retrive IP of the provided host address");
        exit(1);
    }

    addr_list = (struct in_addr **)host->h_addr_list;
    printf("IP address of %s: %s\n", site_address, inet_ntoa(*addr_list[0]));

    if ((sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
    {
        perror("socket error");
        exit(1);
    }
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_addr = *((struct in_addr *)host->h_addr);
    struct timeval start, end;
    double last_latency = 0.0;
    double temp_rtt, temp_latency, temp_bandwidth, curr_latency, prev_rtt = 0.0, new_rtt = -1.0;
    int destination_reached = 0;

    for (int ttl = 1; ttl <= MAX_NO_PACKETS; ttl++)
    {
        printf("Sending ICMP echo request with TTL=%d\n", ttl);
        printf("Finalizing intermediate nodes ...\n\n");
        int t = 20;
        int count = 0;
        int retry_count = 0;
        char prev_ip[INET_ADDRSTRLEN] = "";
        char curr_ip[INET_ADDRSTRLEN] = " ";
        int flag = 0;

        // While loop to finalize intermediate node
        while (t--)
        {
            send_ping(ttl, 0, &start, 0);
            if (recv_ping(ttl, &end) == -1)
            {
                retry_count++;
                if (retry_count >= 5)
                {
                    break;
                }
                continue;
            }
            inet_ntop(AF_INET, &src_addr.sin_addr.s_addr, curr_ip, sizeof(curr_ip));
            if (strcmp(curr_ip, inet_ntoa(*addr_list[0])) == 0)
            {
                destination_reached = 1;
            }
            if (strcmp(prev_ip, "") == 0)
            {
                strcpy(prev_ip, curr_ip);
                count = 1;
            }
            else if (strcmp(curr_ip, prev_ip) == 0)
            {
                count++;
            }
            else
            {
                count = 0;
                strcpy(prev_ip, curr_ip);
            }

            if (count >= 5)
            {
                flag = 1;
                break;
            }
            sleep(1);
        }
        if (flag == 0)
        {
            // Cannot reach intermediate node
            printf("\n * * * \n\n");
            continue;
        }
        printf("Finalized IP of intermediate node:%s\n\n", curr_ip);
        if ((host = gethostbyname(curr_ip)) == NULL)
        {
            herror("Could not retrive IP of the intermediate node");
            exit(1);
        }
        temp_addr.sin_family = AF_INET;
        temp_addr.sin_addr = *((struct in_addr *)host->h_addr);

        temp_bandwidth = -1;
        int newflag = 0;
        // Sending different sized data
        for (int j = 0; j <= 10; j = j + 5)
        {
            for (int i = 0; i < probes_per_link; i++)
            {
                send_ping(ttl, j, &start, flag);
                if (recv_ping(ttl, &end) == -1)
                    continue;
                double temp = (end.tv_sec - start.tv_sec) * (1e6) + (end.tv_usec - start.tv_usec);
                if (newflag == 0)
                {
                    newflag = 1;
                    temp_rtt = temp;
                    new_rtt = temp;
                }
                else if (temp_rtt > temp)
                {
                    temp_rtt = temp;
                }
                else if (new_rtt < temp)
                {
                    new_rtt = temp;
                }
                printf("\033[1;33mSize of data:%d bytes,\tProbe:%d,\tRTT:%0.1f microseconds\n\n\033[0m", j, i + 1, temp);
                sleep(time_difference);
            }
            if (j == 0)
            {
                // Calculate latency by sending zero size msg
                temp_latency = new_rtt / 2.0;
                curr_latency = temp_latency - last_latency;
                last_latency = temp_rtt / 2.0;
            }
            else
            {
                // Calculate bandwidth
                double temp_b = new_rtt - prev_rtt - 2 * curr_latency;
                if (temp_b < eps)
                {
                    temp_b = eps;
                }
                temp_b = j / temp_b;
                if (temp_bandwidth == -1)
                {
                    temp_bandwidth = temp_b;
                }
                else if (temp_bandwidth < temp_b)
                {
                    temp_bandwidth = temp_b;
                }
            }
        }
        prev_rtt = temp_rtt;
        printf("\033[1;33mFor link %d, Latency: %0.1f\n\033[0m", ttl, curr_latency);
        if (curr_latency < 0)
        {
            printf("\033[1;33m*NOTE:Latency is negative due to change in route or congestion level.\n\033[0m");
        }
        printf("\033[1;33mFor link %d, Bandwidth: %0.4fMbps\n\n\033[0m", ttl, temp_bandwidth * 8);

        if (destination_reached == 1)
        {
            printf("Destination Reached !\n");
            break;
        }
    }
    close(sockfd);
    return 0;
}