#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define SERVER_PORT 9000
#define BUFFER_SIZE 4096
#define FILE_DIRECTORY "."

void handle_client(int client_socket) {
    // Gửi danh sách các file cho client
    char file_list[BUFFER_SIZE] = "";
    char response[BUFFER_SIZE] = "";

    DIR *dir;
    struct dirent *ent;

    if ((dir = opendir(FILE_DIRECTORY)) != NULL) {
        int count = 0;
        while ((ent = readdir(dir)) != NULL) {
            if (ent->d_type == DT_REG) {  // Kiểm tra chỉ lấy file
                strcat(file_list, ent->d_name);
                strcat(file_list, "\r\n");
                count++;
            }
        }
        closedir(dir);

        if (count > 0) {
            snprintf(response, sizeof(response), "OK %d\r\n", count);
            strncat(response, file_list, sizeof(response) - strlen(response) - 1);
            strncat(response, "\r\n\r\n", sizeof(response) - strlen(response) - 1);
            send(client_socket, response, strlen(response), 0);
        } else {
            strcpy(response, "ERROR Nofilestodownload\r\n");
            send(client_socket, response, strlen(response), 0);
            close(client_socket);
            return;
        }
    } else {
        strcpy(response, "ERROR Nofilestodownload\r\n");
        send(client_socket, response, strlen(response), 0);
        close(client_socket);
        return;
    }

    // Nhận tên file từ client
    char file_name[BUFFER_SIZE];
    recv(client_socket, file_name, BUFFER_SIZE, 0);
    file_name[strcspn(file_name, "\r\n")] = 0;  // Xóa ký tự xuống dòng

    // Kiểm tra file tồn tại
    char file_path[BUFFER_SIZE];
    snprintf(file_path, sizeof(file_path), "%.*s/%.*s", (int)(sizeof(file_path) - 1), FILE_DIRECTORY, (int)(sizeof(file_path) - strlen(FILE_DIRECTORY) - 2), file_name);
    file_path[sizeof(file_path) - 1] = '\0'; // Đảm bảo kết thúc chuỗi



    FILE *file = fopen(file_path, "rb");

    if (file) {
        // Gửi thông báo OK và kích thước file
        fseek(file, 0, SEEK_END);
        long file_size = ftell(file);
        rewind(file);

        sprintf(response, "OK %ld\r\n", file_size);
        send(client_socket, response, strlen(response), 0);

        // Gửi nội dung file cho client
        char file_buffer[BUFFER_SIZE];
        size_t bytes_read;

        while ((bytes_read = fread(file_buffer, 1, BUFFER_SIZE, file)) > 0) {
            send(client_socket, file_buffer, bytes_read, 0);
        }

        fclose(file);
    } else {
        strcpy(response, "ERROR FileNotFound\r\n");
        send(client_socket, response, strlen(response), 0);
        handle_client(client_socket);
    }

    close(client_socket);
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;

    // Tạo socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }

    // Thiết lập thông tin địa chỉ server
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Gắn thông tin địa chỉ vào socket
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding");
        exit(1);
    }

    // Lắng nghe kết nối từ client
    if (listen(server_socket, 5) < 0) {
        perror("Error listening");
        exit(1);
    }

    printf("Server is listening on port %d\n", SERVER_PORT);

    while (1) {
        // Chấp nhận kết nối từ client
        client_address_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_length);

        if (client_socket < 0) {
            perror("Error accepting connection");
            exit(1);
        }

        printf("Accepted connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

        // Tạo một quá trình xử lý client mới
        if (fork() == 0) {
            close(server_socket);
            handle_client(client_socket);
            exit(0);
        }

        close(client_socket);
    }

    return 0;
}
