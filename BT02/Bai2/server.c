#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <arpa/inet.h>

#define PORT 9000
#define MAXLINE 1024

int main() {
    int sockfd;
    struct sockaddr_in servaddr, cliaddr;
    char buffer[MAXLINE];
    int n;
    socklen_t len;

    // Tạo socket
    sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sockfd < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Thiết lập thông tin server
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(8088);

    // Liên kết socket với địa chỉ và cổng
    if (bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, 5))
    {
        perror("listen() failed");
        return 1;
    }
    int clientAddrLen = sizeof(servaddr);
    int client = accept(sockfd, (struct sockaddr *)&cliaddr, &clientAddrLen);
    printf("Client IP: %s:%d\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
    len = sizeof(cliaddr);
    int count = 0;
    int prev_occurence = 0;

        // Nhận dữ liệu từ client
        n = recvfrom(client, (char *)buffer, MAXLINE, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len);
        buffer[n] = '\0';

        // Tìm số lần xuất hiện của xâu "0123456789" trong dữ liệu nhận được
        char i='0';
        int k=0;
        while (buffer[k]!= '\0') {
            // Xử lý trường hợp khi xâu "0123456789" nằm giữa 2 lần truyền
            if (buffer[k]==i&&i<'9') {
                i++;
            }
            else if(buffer[k]==i&&i=='9'){
                count++;
                i='0';
            }
            k++;
        }
        // Hiển thị số lần xuất hiện
        printf("Number of occurrences of \"0123456789\": %d\n", count);


    close(sockfd);
    return 0;
}
