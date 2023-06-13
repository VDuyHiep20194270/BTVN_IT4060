#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024
#define MAX_CLIENTS 2

typedef struct {
    int client_socket;
    pthread_t thread;
} ClientInfo;

ClientInfo client_queue[MAX_CLIENTS];
pthread_mutex_t mutex;
int num_clients = 0;

void* client_thread(void* arg) {
    int client_index = *((int*)arg);
    int partner_index = (client_index + 1) % MAX_CLIENTS;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    while ((bytes_read = read(client_queue[client_index].client_socket, buffer, BUFFER_SIZE)) > 0) {
        pthread_mutex_lock(&mutex);
        write(client_queue[partner_index].client_socket, buffer, bytes_read);
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_lock(&mutex);
    printf("Client %d disconnected\n", client_index);
    close(client_queue[client_index].client_socket);
    num_clients--;
    pthread_mutex_unlock(&mutex);

    return NULL;
}

int main() {
    int server_socket, client_socket;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_length;

    pthread_mutex_init(&mutex, NULL);

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        perror("Error creating socket");
        exit(1);
    }

    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(9000);

    if (bind(server_socket, (struct sockaddr*)&server_address, sizeof(server_address)) < 0) {
        perror("Error binding socket");
        exit(1);
    }

    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Error listening for connections");
        exit(1);
    }

    printf("Server started, waiting for connections...\n");

    while (1) {
        client_address_length = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr*)&client_address, &client_address_length);
        if (client_socket < 0) {
            perror("Error accepting connection");
            exit(1);
        }

        pthread_mutex_lock(&mutex);
        if (num_clients < MAX_CLIENTS) {
            client_queue[num_clients].client_socket = client_socket;
            printf("New connection accepted from %s:%hu\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            num_clients++;
            pthread_create(&client_queue[num_clients - 1].thread, NULL, client_thread, &num_clients);
        } else {
            printf("Connection rejected from %s:%hu (maximum number of clients reached)\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
            close(client_socket);
        }
        pthread_mutex_unlock(&mutex);
    }

    pthread_mutex_destroy(&mutex);
    close(server_socket);

    return 0;
}
