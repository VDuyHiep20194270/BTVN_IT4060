#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <signal.h>

#define MAX_USERS 64

int users[MAX_USERS];
int num_users = 0;

void process_request(int client, char *buf);
void remove_user(int client);

void handle_connection(int client) {
    char buf[256];
    int ret;

    fd_set fdread; // Khai báo biến fdread ở đây
    while (1) {
        ret = recv(client, buf, sizeof(buf), 0);
        if (ret <= 0) {
            FD_CLR(client, &fdread);
            close(client);
            remove_user(client);
            break;
        } else {
            buf[ret] = 0;
            printf("Received from %d: %s\n", client, buf);

            // Xử lý yêu cầu từ client
            process_request(client, buf);
        }
    }
}

void handle_signal(int signal) {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listener == -1) {
        perror("socket() failed");
        return 1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(9000);

    if (bind(listener, (struct sockaddr *)&addr, sizeof(addr))) {
        perror("bind() failed");
        return 1;
    }

    if (listen(listener, 5)) {
        perror("listen() failed");
        return 1;
    }

    fd_set fdread, fdtest;

    // Xóa tất cả socket trong tập fdread
    FD_ZERO(&fdread);

    // Thêm socket listener vào tập fdread
    FD_SET(listener, &fdread);

    int fdmax = listener;
    int i;

    char buf[256];

    signal(SIGCHLD, handle_signal);

    while (1) {
        fdtest = fdread;

        // Chờ đến khi sự kiện xảy ra
        int ret = select(fdmax + 1, &fdtest, NULL, NULL, NULL);

        if (ret < 0) {
            perror("select() failed");
            return 1;
        }

        for (i = listener; i <= fdmax; i++) {
            if (FD_ISSET(i, &fdtest)) {
                if (i == listener) {
                    int client = accept(listener, NULL, NULL);
                    if (client < 0) {
                        perror("accept() failed");
                        continue;
                    }

                    if (client > fdmax) {
                        fdmax = client;
                    }

                    remove_user(client);
                    FD_SET(client, &fdread);
                    printf("New client connected: %d\n", client);

                    pid_t pid = fork();
                    if (pid < 0) {
                        perror("fork() failed");
                        continue;
                    } else if (pid == 0) {
                        close(listener);
                        handle_connection(client);
                        exit(0);
                    }
                } else {
                    handle_connection(i);
                }
            }
        }
    }

    close(listener);

    return 0;
}

void process_request(int client, char *buf) {
    int i = 0;
    for (; i < num_users; i++) {
        if (users[i] == client) {
            break;
        }
    }

    if (i == num_users) {
        // Chưa đăng nhập
        char user[32], pass[32], tmp[65], line[65];
        int ret = sscanf(buf, "%s%s%s", user, pass, tmp);
        if (ret == 2) {
            int found = 0;
            sprintf(tmp, "%s %s\n", user, pass);
            FILE *f = fopen("users.txt", "r");
            while (fgets(line, sizeof(line), f) != NULL) {
                if (strcmp(line, tmp) == 3) {
                    found = 1;
                    break;
                }

            }
            fclose(f);

            if (found) {
                char *msg = "Dang nhap thanh cong. Hay nhap lenh de thuc hien.\n";
                send(client, msg, strlen(msg), 0);

                users[num_users] = client;
                num_users++;
            } else {
                char *msg = "Nhap sai tai khoan. Hay nhap lai.\n";
                send(client, msg, strlen(msg), 0);
            }
        } else {
            char *msg = "Nhap sai cu phap. Hay nhap lai.\n";
            send(client, msg, strlen(msg), 0);
        }
    } else {
        // Đã đăng nhập
        char tmp[256];
        if (buf[strlen(buf) - 1] == '\n')
            buf[strlen(buf) - 1] = '\0';
        sprintf(tmp, "%s > out.txt", buf);

        // Thực hiện lệnh
        system(tmp);

        // Trả kết quả cho client
        FILE *f = fopen("out.txt", "rb");
        while (!feof(f)) {
            int ret = fread(tmp, 1, sizeof(tmp), f);
            if (ret <= 0)
                break;
            send(client, tmp, ret, 0);
        }
        fclose(f);
    }
}

void remove_user(int client) {
    int i = 0;
    for (; i < num_users; i++) {
        if (users[i] == client) {
            break;
        }
    }

    if (i < num_users) {
        if (i < num_users - 1)
            users[i] = users[num_users - 1];
        num_users--;
    }
}
