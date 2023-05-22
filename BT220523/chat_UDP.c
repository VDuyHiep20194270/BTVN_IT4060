#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>


#define MAX_BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <receiver IP> <receiver port> <listen port>\n", argv[0]);
        return 1;
    }

    // Parse command line arguments
    const char *receiverIP = argv[1];
    int receiverPort = atoi(argv[2]);
    int listenPort = atoi(argv[3]);

    // Create socket for receiving messages
    int recvSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (recvSocket < 0) {
        perror("Failed to create receive socket");
        return 1;
    }

    // Bind the socket to the listen port
    struct sockaddr_in recvAddr;
    memset(&recvAddr, 0, sizeof(recvAddr));
    recvAddr.sin_family = AF_INET;
    recvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    recvAddr.sin_port = htons(listenPort);
    if (bind(recvSocket, (struct sockaddr *)&recvAddr, sizeof(recvAddr)) < 0) {
        perror("Failed to bind receive socket");
        close(recvSocket);
        return 1;
    }

    // Create socket for sending messages
    int sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (sendSocket < 0) {
        perror("Failed to create send socket");
        close(recvSocket);
        return 1;
    }

    // Set up the receiver address
    struct sockaddr_in receiverAddr;
    memset(&receiverAddr, 0, sizeof(receiverAddr));
    receiverAddr.sin_family = AF_INET;
    receiverAddr.sin_addr.s_addr = inet_addr(receiverIP);
    receiverAddr.sin_port = htons(receiverPort);

    printf("Chat program started. Type 'quit' to exit.\n");

    char message[MAX_BUFFER_SIZE];
    int maxSocket = (recvSocket > sendSocket) ? recvSocket : sendSocket;
    fd_set readSet;

    while (1) {
        // Clear the socket set and add the sockets to it
        FD_ZERO(&readSet);
        FD_SET(recvSocket, &readSet);
        FD_SET(0, &readSet); // 0 represents stdin
        int activity = select(maxSocket + 1, &readSet, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Error in select");
            break;
        }

        // If receive socket is set, receive and display message
        if (FD_ISSET(recvSocket, &readSet)) {
            struct sockaddr_in senderAddr;
            socklen_t senderAddrLen = sizeof(senderAddr);
            int bytesRead = recvfrom(recvSocket, message, MAX_BUFFER_SIZE, 0,
                                     (struct sockaddr *)&senderAddr, &senderAddrLen);
            if (bytesRead > 0) {
                message[bytesRead] = '\0';
                printf("Received message: %s\n", message);
            }
        }

        // If stdin is set, read and send message
        if (FD_ISSET(0, &readSet)) {
            fgets(message, MAX_BUFFER_SIZE, stdin);
            int messageLen = strlen(message);

            // Remove trailing newline character
            if (message[messageLen - 1] == '\n') {
                message[messageLen - 1] = '\0';
                messageLen--;
            }

            // Check if user wants to quit
            if (strcmp(message, "quit") == 0) {
                break;
            }

            // Send the message to the receiver
            if (sendto(sendSocket, message, messageLen, 0,
                       (struct sockaddr *)&receiverAddr, sizeof(receiverAddr)) < 0) {
                perror("Failed to send message");
                break;
            }
        }
    }

    // Close the sockets
    close(recvSocket);
    close(sendSocket);

    return 0;
}
    