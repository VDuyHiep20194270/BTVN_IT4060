#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <time.h>

#define PORT 9000
#define MAX_CLIENTS 5
#define BUFFER_SIZE 256

void handle_connection(int client);
void get_current_time(char *format, char *result);

int main() {
    int listener = socket(AF_INET, SOCK_STREAM, 0);
    if (listener == -1) {
        perror("socket() failed");
        exit(1);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(PORT);

    if (bind(listener, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind() failed");
        exit(1);
    }

    if (listen(listener, MAX_CLIENTS) == -1) {
        perror("listen() failed");
        exit(1);
    }

    printf("Time server started. Listening on port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addrlen = sizeof(client_addr);
        int client = accept(listener, (struct sockaddr *)&client_addr, &client_addrlen);
        if (client == -1) {
            perror("accept() failed");
            continue;
        }

        printf("New client connected: %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        // Fork a child process to handle the connection
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork() failed");
            close(client);
            continue;
        }

        if (pid == 0) {
            // Child process
            close(listener);
            handle_connection(client);
            close(client);
            exit(0);
        } else {
            // Parent process
            close(client);
            waitpid(pid, NULL, WNOHANG); // Clean up zombie processes
        }
    }

    close(listener);
    return 0;
}

void handle_connection(int client) {
    char buffer[BUFFER_SIZE];
    int ret = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (ret <= 0) {
        perror("recv() failed");
        return;
    }

    buffer[ret] = '\0';

    // Parse the command from the client
    char command[BUFFER_SIZE];
    char format[BUFFER_SIZE];
    int parsed_args = sscanf(buffer, "%s %s", command, format);
    if (parsed_args != 2 || strcmp(command, "GET_TIME") != 0) {
        char *error_msg = "Invalid command. Usage: GET_TIME [format]\n";
        send(client, error_msg, strlen(error_msg), 0);
        return;
    }

    char result[BUFFER_SIZE];
    get_current_time(format, result);
    send(client, result, strlen(result), 0);
}

void get_current_time(char *format, char *result) {
    time_t current_time = time(NULL);
    struct tm *time_info = localtime(&current_time);
    
    // Kiểm tra định dạng thời gian và định dạng kết quả tương ứng
    if (strcmp(format, "dd/mm/yyyy") == 0) {
        strftime(result, BUFFER_SIZE, "%d/%m/%Y", time_info);
    } else if (strcmp(format, "dd/mm/yy") == 0) {
        strftime(result, BUFFER_SIZE, "%d/%m/%y", time_info);
    } else if (strcmp(format, "mm/dd/yyyy") == 0) {
        strftime(result, BUFFER_SIZE, "%m/%d/%Y", time_info);
    } else if (strcmp(format, "mm/dd/yy") == 0) {
        strftime(result, BUFFER_SIZE, "%m/%d/%y", time_info);
    } else {
        // Định dạng không hợp lệ, trả về chuỗi rỗng
        result[0] = '\0';
    }
}
