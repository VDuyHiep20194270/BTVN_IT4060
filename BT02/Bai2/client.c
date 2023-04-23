#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUF_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <Server IP> <Port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    // Create a socket
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(8088);

    int ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("connect() failed1");
        return 1;
    }

    // Read input file
    FILE *fp = fopen("input.txt", "r");
    if (fp == NULL) {
        perror("fopen() failed2");
        exit(EXIT_FAILURE);
    }
    char buf[BUF_SIZE];
    size_t nread;
    while ((nread = fread(buf, 1, sizeof(buf), fp)) > 0) {
        // Send data to server
        if (send(client, buf, nread, 0) != nread) {
            perror("send() failed3");
            exit(EXIT_FAILURE);
        }
    }

    // Close the socket
    close(client);
    fclose(fp);
    return 0;
}
