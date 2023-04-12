#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024

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
    FILE *hello_file = fopen(argv[2], "r");
    char buffer[BUFFER_SIZE];
    int bytes_read = 0;
    while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, hello_file)) > 0) {
        if (send(client, buffer, bytes_read, 0) < 0) {
            perror("Failed to send greeting to client");
            exit(EXIT_FAILURE);
        }
    }
    fclose(hello_file);

    char buf[256];

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
    FILE *output_file = fopen(argv[3], "w");
    while (1) {
        bytes_read = recv(client, buffer, BUFFER_SIZE, 0);
        if (bytes_read <= 0) {
            break;
        }
        fwrite(buffer, 1, bytes_read, output_file);
    }
    fclose(output_file);
    
    close(client);
    close(listener);
}
