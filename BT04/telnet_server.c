#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

void read_credentials(const char* file_path, char** credentials, int* count) {
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        printf("Error opening file %s\n", file_path);
        exit(1);
    }

    char buffer[BUFFER_SIZE];
    int i = 0;

    while (fgets(buffer, BUFFER_SIZE, file) != NULL) {
        char* newline = strchr(buffer, '\n');
        if (newline != NULL) {
            *newline = '\0';
        }

        credentials[i] = strdup(buffer);
        i++;
    }

    *count = i;

    fclose(file);
}

int authenticate(const char* userpass, char** credentials, int count) {
    for (int i = 0; i < count; i++) {
        // printf("%s\n",credentials[i]);
        // printf("%s\n",userpass);
        // printf("%d\n",strcmp(userpass,credentials[i]));
        if (strcmp(userpass,credentials[i])==-13) {
            return 1; // Authentication successful
        }
    }

    return 0; // Authentication failed
}

void handle_command(int client_socket, const char* command) {
    char buffer[BUFFER_SIZE];
    FILE* pipe = popen(command, "r");
    if (pipe == NULL) {
        printf("Error executing command\n");
        strcpy(buffer, "Error executing command\n");
    } else {
        while (fgets(buffer, BUFFER_SIZE, pipe) != NULL) {
            send(client_socket, buffer, strlen(buffer), 0);
        }
        pclose(pipe);
    }

    send(client_socket, buffer, strlen(buffer), 0);
}

int main() {
    // Read credentials from file
    char* credentials[MAX_CLIENTS];
    int credentials_count;
    read_credentials("credentials.txt", credentials, &credentials_count);

    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Error creating server socket");
        exit(1);
    }

    // Set server socket options
    int opt = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("Error setting server socket options");
        exit(1);
    }

    // Bind server socket to a port
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(9000); // Telnet port

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) == -1) {
        perror("Error binding server socket");
        exit(1);
    }

    // Listen for incoming connections
    if (listen(server_socket, MAX_CLIENTS) == -1) {
        perror("Error listening for connections");
        exit(1);
    }

    printf("Telnet server started on port 23\n");

    // Set of client sockets
    fd_set read_fds;
    int client_sockets[MAX_CLIENTS];
    memset(client_sockets, 0, sizeof(client_sockets));

    while (1) {
        // Clear the socket set
        FD_ZERO(&read_fds);

        // Add server socket to the set
        FD_SET(server_socket, &read_fds);

        // Add client sockets to the set
        int max_fd = server_socket;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_socket = client_sockets[i];
            if (client_socket > 0) {
                FD_SET(client_socket, &read_fds);
                if (client_socket > max_fd) {
                    max_fd = client_socket;
                }
            }
        }

        // Wait for activity on any of the sockets
        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) == -1) {
            perror("Error in select");
            exit(1);
        }

        // Check if there is a new connection on the server socket
        if (FD_ISSET(server_socket, &read_fds)) {
            struct sockaddr_in client_address;
            socklen_t client_address_size = sizeof(client_address);
            int new_client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_size);
            if (new_client_socket == -1) {
                perror("Error accepting new connection");
                exit(1);
            }

            printf("New connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));

            // Add new client socket to the array
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_client_socket;
                    break;
                }
            }
        }

        // Check if there is data to be read from any of the client sockets
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_socket = client_sockets[i];
            if (FD_ISSET(client_socket, &read_fds)) {
                char buffer[BUFFER_SIZE];
                ssize_t num_bytes = recv(client_socket, buffer, BUFFER_SIZE - 1, 0);
                printf("%ld\n",num_bytes);
                if (num_bytes == -1) {
                    perror("Error receiving data");
                    exit(1);
                } else if (num_bytes == 0) {
                    // Connection closed by client
                    printf("Connection closed from %d\n", client_socket);
                    close(client_socket);
                    client_sockets[i] = 0;
                } else {
                    buffer[num_bytes] = '\0';

                    if (strstr(buffer, "\n") != NULL) {
                        // Remove trailing newline character
                        buffer[num_bytes -1] = '\0';
                    }
                    if (authenticate(buffer, credentials, credentials_count)) {
                        send(client_socket, "Login successful!\n", strlen("Login successful!\n"), 0);
                    } else {
                        send(client_socket, "Invalid username or password\n", strlen("Invalid username or password\n"), 0);
                        close(client_socket);
                        client_sockets[i] = 0;
                    }
                }
            }
        }
    }

    // Clean up
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client_sockets[i] > 0) {
            close(client_sockets[i]);
        }
    }

    close(server_socket);

    return 0;
}
