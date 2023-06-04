#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/wait.h>

#define NUM_CHILDREN 5

void handle_connection(int client);

int main()
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
    addr.sin_port = htons(9000);

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

    fd_set fdread, fdtest;

    FD_ZERO(&fdread);
    FD_SET(listener, &fdread);

    int children[NUM_CHILDREN];
    int i, maxfd = listener;

    for (i = 0; i < NUM_CHILDREN; i++)
    {
        pid_t pid = fork();

        if (pid == 0)
        {
            while (1)
            {
                fdtest = fdread;

                int ret = select(maxfd + 1, &fdtest, NULL, NULL, NULL);

                if (ret < 0)
                {
                    perror("select() failed");
                    return 1;
                }

                if (FD_ISSET(listener, &fdtest))
                {
                    int client = accept(listener, NULL, NULL);
                    printf("New client connected: %d\n", client);
                    handle_connection(client);
                    close(client);
                }
            }

            return 0;
        }
        else if (pid > 0)
        {
            children[i] = pid;
        }
        else
        {
            perror("fork() failed");
            return 1;
        }
    }

    // Chờ tất cả các child process kết thúc
    for (i = 0; i < NUM_CHILDREN; i++)
    {
        waitpid(children[i], NULL, 0);
    }

    close(listener);

    return 0;
}

void handle_connection(int client)
{
    char buf[256];
    int ret = recv(client, buf, sizeof(buf), 0);
    buf[ret] = 0;
    puts(buf);

    char *msg = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n<html><body><h1>Xin chao cac ban</h1></body></html>";
    send(client, msg, strlen(msg), 0);
}
