#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <ctype.h>
#include <stdbool.h>

void removeExtraSpaces(char *str)
{
    // Bỏ các ký tự dư thừa ở đầu xâu
    while (*str == ' ')
    {
        ++str;
    }

    // Bỏ các ký tự dư thừa ở cuối xâu
    char *end = str;
    while (*end != '\0')
    {
        ++end;
    }
    --end;
    while (end > str && *end == ' ')
    {
        --end;
    }
    *(end + 1) = '\0';

    // Bỏ các ký tự dư thừa ở giữa xâu
    char *p = str;
    bool spaceFound = false;
    while (*str)
    {
        if (*str == ' ')
        {
            if (!spaceFound)
            {
                spaceFound = true;
                *p++ = *str++;
            }
            else
            {
                ++str;
            }
        }
        else
        {
            spaceFound = false;
            *p++ = *str++;
        }
    }
    *p = '\0';
}

void capitalizeFirstLetters(char *str)
{
    bool newWord = true;
    while (*str)
    {
        if (isspace(*str))
        {
            newWord = true;
        }
        else
        {
            if (newWord)
            {
                *str = toupper(*str);
                newWord = false;
            }
            else
            {
                *str = tolower(*str);
            }
        }
        ++str;
    }
}

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

    // Xóa tất cả socket trong tập fdread
    FD_ZERO(&fdread);

    // Thêm socket listener vào tập fdread
    FD_SET(listener, &fdread);

    char buf[256];
    struct timeval tv;

    int users[64];      // Mang socket client da dang nhap
    char *user_ids[64]; // Mang id client da dang nhap
    int num_users = 0;  // So luong client da dang nhap

    while (1)
    {
        fdtest = fdread;

        // Thiet lap thoi gian cho
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        // Chờ đến khi sự kiện xảy ra
        int ret = select(FD_SETSIZE, &fdtest, NULL, NULL, NULL);

        if (ret < 0)
        {
            perror("select() failed");
            return 1;
        }

        if (ret == 0)
        {
            printf("Timed out!!!\n");
            continue;
        }

        for (int i = listener; i < FD_SETSIZE; i++)
        {
            if (FD_ISSET(i, &fdtest))
            {
                if (i == listener)
                {
                    int client = accept(listener, NULL, NULL);
                    if (client < FD_SETSIZE)
                    {
                        FD_SET(client, &fdread);
                        printf("New client connected: %d\n", client);
                    }
                    else
                    {
                        // Dang co qua nhieu ket noi
                        close(client);
                    }
                }
                else
                {
                    int ret = recv(i, buf, sizeof(buf), 0);
                    if (ret <= 0)
                    {
                        FD_CLR(i, &fdread);
                        close(i);
                    }
                    else
                    {
                        buf[ret] = 0;
                        printf("Received from %d: %s\n", i, buf);

                        int client = i;

                        // Kiem tra trang thai dang nhap
                        int j = 0;
                        for (; j < num_users; j++)
                            if (users[j] == client)
                                break;

                        if (j == num_users) // Chua ket noi truoc do
                        {
                            users[num_users] = client;
                            num_users++;
                            char msg[100];
                            sprintf(msg, "Xin chào hiện tại đang có %d clients đang kết nối", num_users);
                            msg[strlen(msg)] = '\0';
                            send(client, msg, strlen(msg), 0);
                        }
                        else // Da dang nhap
                        {
                            char *newline = strchr(buf, '\n');
                            if (newline != NULL)
                            {
                                *newline = '\0';
                            }

                            // Chuẩn hóa xâu
                            removeExtraSpaces(buf);
                            capitalizeFirstLetters(buf);
                            send(client, buf, strlen(buf), 0);
                        }
                    }
                }
            }
        }
    }

    close(listener);

    return 0;
}