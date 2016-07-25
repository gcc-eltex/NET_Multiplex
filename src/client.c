#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define SRV_PORT    3144        // Порт сервера
#define SRV_ADDR    "127.0.0.1" // IP адрес сервера
#define MSG_MAXLEN  64          // Максимальная длина сообщения


int main(int argc, void *argv[])
{
    int                 sock;               // Сокет клиента    
    struct sockaddr_in  addr;               // Данные сервера
    char                msg[MSG_MAXLEN];    // Передаваемое / получаемое
                                            // сообщение
    // Создаем сокет клиента
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1){
        perror("socket");
        exit(-1);
    }

    // Подключаемся к серверу
    addr.sin_family = AF_INET;
    addr.sin_port = htons(SRV_PORT);
    addr.sin_addr.s_addr = inet_addr(SRV_ADDR);
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == -1){
        perror("connect");
        close(sock);
        exit(-1);
    }

    while (1){
        // Считываем и отправляем
        printf("send: ");
        scanf("%s", msg);

        if (send(sock, msg, strlen(msg) + 1, 0) == -1)
            perror("send");

        // Условие выхода
        if (!strcmp(msg, "exit"))
            break;

        if (recv(sock, msg, MSG_MAXLEN, 0) == -1)
            perror("send");
        else
            printf("recv: %s\n", msg);
    }

    // Завершение
    close(sock);
    exit(0);
}
