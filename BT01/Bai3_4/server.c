#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define BUFFER_SIZE 1024

void log_data(char *client_ip, char *data, char *log_file)
{
    FILE *fp;
    time_t now;
    char time_str[64];

    now = time(NULL);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    fp = fopen(log_file, "a");
    fprintf(fp, "%s %s %s\n", client_ip, time_str, data);
    fclose(fp);
}

int main(int argc, char *argv[])
{
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1)
    {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(atoi(argv[1]));

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr)))
    {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5))
    {
        perror("listen() failed");
        return 1;
    }

    struct sockaddr_in clientAddr;
    int clientAddrLen = sizeof(addr);

    int client = accept(listener, (struct sockaddr *)&clientAddr, &clientAddrLen);
    printf("Client IP: %s:%d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));

    // char *msg = "Hello client\n";
    // send(client, msg, strlen(msg), 0);
    // Send greeting to client
    // FILE *hello_file = fopen(argv[2], "r");
    // char buffer[BUFFER_SIZE];
    // int bytes_read = 0;
    // while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, hello_file)) > 0) {
    //     if (send(client, buffer, bytes_read, 0) < 0) {
    //         perror("Failed to send greeting to client");
    //         exit(EXIT_FAILURE);
    //     }
    // }
    // fclose(hello_file);

    // while (1)
    // {
    //     int ret = recv(client, buf, sizeof(buf), 0);
    //     if (ret <= 0)
    //     {
    //         printf("Ket noi bi dong.\n");
    //         break;
    //     }
    //     if (ret < sizeof(buf))
    //         buf[ret] = 0;

    //     printf("%d bytes received\n", ret);
    //     printf("%s\n", buf);
    // }
    // Receive data from client and write to file
    while (1)
    {
        // Get client IP address
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(clientAddr.sin_addr), client_ip, INET_ADDRSTRLEN);
        // Get current time
        time_t now;
        struct tm *local_time;
        time(&now);
        local_time = localtime(&now);
        char timestamp[20];
        strftime(timestamp, 20, "%Y-%m-%d %H:%M:%S", local_time);
        // Receive data from client
        char buf[BUFFER_SIZE];
        ssize_t num_bytes = recv(client, buf, sizeof(buf), 0);
        if (num_bytes <= 0)
        {
            perror("Error receiving data from client");
            return 1;
        }
        FILE *fp = fopen(argv[2], "a");
        if (fp == NULL)
        {
            perror("Error opening log file");
            return 1;
        }

        fprintf(fp, "%s %s %s\n", client_ip, timestamp, buf);
        printf("%s %s %s", client_ip, timestamp, buf);

        fclose(fp);
    }

    close(client);
    close(listener);
}
