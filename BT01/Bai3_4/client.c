#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define BUFF_SIZE 1024

typedef struct
{
    char mssv[10];
    char hoten[50];
    char ngaysinh[15];
    float diemtb;
} SinhVien;

int main(int argc, char *argv[])
{
    int client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(argv[1]);
    addr.sin_port = htons(atoi(argv[2]));

    int ret = connect(client, (struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
    {
        perror("connect() failed");
        return 1;
    }

    char buf[BUFF_SIZE];
    // ret = recv(client, buf, sizeof(buf), 0);
    // if (ret <= 0)
    // {
    //     printf("Connection closed\n");
    //     return 1;
    // }

    // if (ret < sizeof(buf))
    //     buf[ret] = 0;
    // printf("%d bytes received: %s\n", ret, buf);

    while (1)
    {
        printf("Enter string:");
        fgets(buf, sizeof(buf), stdin);

        // send(client, buf, strlen(buf), 0);

        if (strncmp(buf, "exit", 4) == 0)
            break;

        // Nhập thông tin sinh viên
        SinhVien sv;
        printf("MSSV: ");
        fgets(sv.mssv, 10, stdin);
        printf("Ho ten: ");
        fgets(sv.hoten, 50, stdin);
        printf("Ngay sinh: ");
        fgets(sv.ngaysinh, 15, stdin);
        printf("Diem trung binh: ");
        scanf("%f", &sv.diemtb);
        getchar(); // Nhập vào kí tự '\n' ở cuối

        // Đóng gói dữ liệu
        char buf[BUFF_SIZE];
        int offset = 0;
        memcpy(buf + offset, sv.mssv, strlen(sv.mssv));
        offset += strlen(sv.mssv);
        memcpy(buf + offset, sv.hoten, strlen(sv.hoten));
        offset += strlen(sv.hoten);
        memcpy(buf + offset, sv.ngaysinh, strlen(sv.ngaysinh));
        offset += strlen(sv.ngaysinh);
        memcpy(buf + offset, &sv.diemtb, sizeof(float));
        offset += sizeof(float);

        // Gửi dữ liệu tới server
        if (send(client, buf, offset, 0) < 0)
        {
            perror("Failed to send data to server");
            return 1;
        }
    }

    close(client);
}