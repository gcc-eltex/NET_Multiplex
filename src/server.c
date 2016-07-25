#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <pthread.h>

#define SERVER_IP   "127.0.0.1"     // IP сервера
#define SERVER_PORT 3141            // Порт сервера
#define MAX_MSGLEN  64
#define MAX_CLT     100             // Число клиентов, обрабатываемое одним
                                    // потоком

void thread_clients(void *argv);
void error_completion(int close_fd, char *emsg);

int main()
{
    int                 sock;        // Слушающий сокет
    struct sockaddr_in  addr_srv;    // Данные сервера
    struct sockaddr_in  addr_clt;    // Данные клиента

    addr_srv.sin_family = AF_INET;
    addr_srv.sin_port = htons(SERVER_PORT);
    addr_srv.sin_addr.s_addr = inet_addr(SERVER_IP);
    if ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
        error_completion(sock, "ERROR socket");

    if (bind(sock, (struct sockaddr *)&addr_srv, sizeof(struct sockaddr_in)) == -1)
        error_completion(sock, "ERROR bind");
    listen(sock, 10);

    thread_clients((void *)&sock);
  
    close(sock);
    exit(0);
}

/*
 * Запускается в новом потоке. Обрабатывает MAX_CLT клиентов. При достижении
 * максимального количества клиентов создает новый поток
 *
 * @param argv   слушающий сокет сервера
 */
void thread_clients(void *argv)
{
    int                 cnt;        // Текущее количество обслуживаемых
                                    // клиентов в потоке
    int                 epfd;       // Дескриптор epoll
    struct epoll_event  ev;         // Событие для отслкживания нового клиента
    struct epoll_event  *events;    // Произошедшие события
    int                 nev;        // Количество произошедших событий
    int                 sock;       // Сдушающий сокет
    int                 sock_clt;   // Сокет подключенного клиента
    int                 addr_size;  // Размер struct sockaddr_in
    struct sockaddr_in  addr;       // Данные нового клиента
    char                msg[64];    // Получаемое/отправляемое сообщение
    
    cnt = 0;
    sock = *((int *)argv);
    addr_size = sizeof(struct sockaddr_in *);
    if ((epfd = epoll_create(MAX_CLT)) == -1)
        error_completion(sock, "ERROR epoll_create");
    printf("Run thread. Listen socket: %d\n", sock);

    /* Добавляем слушающий сокет для отслеживания, чтобы новые клиенты
     * могли подключиться и обрабатываться в данном потоке
     */
    ev.events = EPOLLIN;
    ev.data.fd = sock;
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock, &ev) == -1)
        error_completion(sock, "ERROR epoll_ctl");

    // Отслеживаем изменения
    while (1){
        if ((nev = epoll_wait(epfd, events, MAX_CLT, -1)) == -1)
            error_completion(sock, "ERROR epoll_wait");
        
        for (int i = 0; i < nev; i++){
            /* Запрос на подключение нового клиента. Принимаем его и добавляем 
             * новый сокет для отслеживания
             */
            if (events[i].data.fd == sock){
                sock_clt = accept(sock, (struct sockaddr *)&addr, &addr_size);
                if (sock_clt == -1)
                    error_completion(sock, "ERROR accept");

                ev.events = EPOLLIN;
                ev.data.fd = sock_clt;
                if (epoll_ctl(epfd, EPOLL_CTL_ADD, sock_clt, &ev) == -1)
                    error_completion(sock, "ERROR epoll_ctl add");
                
                /* Проверяем, достигнуто ли максимальное количество клиентов.
                 * Если да, то необходимо удалить слушающий сокет 
                 * из отслеживаемых и создать новый поток, который станет 
                 * принимать новых клиентов
                 */
                cnt++;
                if (cnt > MAX_CLT){
                    pthread_t tid;
                    // Удаляем слушающий сокет из отслеживаемых в этом потоке
                    if (epoll_ctl(epfd, EPOLL_CTL_DEL, sock_clt, NULL) == -1)
                        error_completion(sock, "ERROR epoll_ctl delete");
                    if (pthread_create(&tid, 0, (void *)thread_clients, argv) 
                                        == -1)
                        error_completion(sock, "ERROR pthread_create");
                }
                continue;
            }
            /* В остальных случаях это просто сообщение, на которое 
             * отправляется echo ответ
             */
            if (recv(events[i].data.fd, msg, 64, 0) == -1)
                error_completion(sock, "ERROR recv");
            if (send(events[i].data.fd, msg, 64, 0) == -1)
                error_completion(sock, "ERROR recv");
        }
    }
}

/*
 * Завершает программу с ошибкой
 *
 * @param closefd   закрываемый слушающий сокет сервера
 * @param emsg      сообщение выводимое при ошибке
 */
void error_completion(int closefd, char *emsg)
{
    close(closefd);
    perror(emsg);
    exit(-1);
}