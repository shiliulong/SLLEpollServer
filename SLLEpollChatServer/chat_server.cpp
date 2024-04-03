// g++ chat_server.cpp -g -O2 -o chat_server.exe

#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <vector>
#include <map>

#define BUFFER_SIZE 512
 char ipstr[128];


int g_kInitEventListSize = 16;
std::vector<struct epoll_event> g_revents(g_kInitEventListSize);

struct client_data
{
    int fd;
    char* write_buf;
    char buf[BUFFER_SIZE];
};

std::map<int, client_data*> g_mapClientData;

int setnonblocking(int fd)
{
    int old_option = fcntl(fd, F_GETFL);
    int new_option = old_option | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_option);
    return old_option;
}

void addfd(int epollfd, int fd, uint32_t events = EPOLLIN)
{
    epoll_event event;
    event.data.fd = fd;
    event.events = events;
    int retCtl = epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event);
    printf("register other client epolladd retCtl:%d, errmsg:%s \n", retCtl, strerror(errno));
    setnonblocking(fd);
}

void handle_event(epoll_event* events, int number, int epollfd, int listenfd)
{
    char buf[BUFFER_SIZE];
    for (int i = 0; i < number; ++i)
    {
        int sockfd = events[i].data.fd;
        if (sockfd == listenfd)
        {
            struct sockaddr_in client_address;
            socklen_t client_addrlength = sizeof(client_address);
            int connfd = accept(listenfd, (struct sockaddr*)&client_address, &client_addrlength);
            printf("client ip %s\tport %d\n",
                inet_ntop(AF_INET, (struct sockaddr*)&client_address.sin_addr.s_addr, ipstr, sizeof(ipstr)), ntohs(client_address.sin_port));
            addfd(epollfd, connfd);

            client_data* pClientData = new client_data();
            pClientData->fd = connfd;
            g_mapClientData[connfd] = pClientData;
        }
        else if (events[i].events & EPOLLIN)
        {
            printf("event trigger once\n");

            auto msgClient = g_mapClientData[sockfd];
            memset(msgClient->buf, '\0', BUFFER_SIZE);
            int ret = recv(sockfd, msgClient->buf, BUFFER_SIZE - 1, 0);
            if (ret <= 0)
            {
                int retCtl = epoll_ctl(epollfd, EPOLL_CTL_DEL, sockfd, nullptr);
                printf("register other client epolldel retCtl:%d, errmsg:%s \n", retCtl, strerror(errno));
                close(sockfd);

                g_mapClientData.erase(sockfd);
                delete msgClient;
                msgClient = nullptr;
                continue;
            }
            else
            {
                for (auto it = g_mapClientData.begin();
                    it != g_mapClientData.end(); ++it)
                {
                    if (it->second->fd == sockfd)
                    {
                        continue;
                    }
                    auto otherClient = it->second;

                    epoll_event event;
                    event.data.fd = otherClient->fd;
                    event.events = EPOLLOUT;
                    int retCtl = epoll_ctl(epollfd, EPOLL_CTL_MOD, otherClient->fd, &event);
                    printf("register other client epollout retCtl:%d, errmsg:%s \n", retCtl, strerror(errno));

                    otherClient->write_buf = msgClient->buf;
                }
            }
            printf("get %d bytes of content:%s\n", ret, buf);
        }
        else if (events[i].events & EPOLLOUT)
        {
            auto msgClient = g_mapClientData[sockfd];
            if (msgClient->write_buf == nullptr)
            {
                continue;
            }

            send(sockfd, msgClient->write_buf, strlen(msgClient->write_buf), 0);
            msgClient->write_buf = nullptr;

            epoll_event event;
            event.data.fd = msgClient->fd;
            event.events = EPOLLIN;
            epoll_ctl(epollfd, EPOLL_CTL_MOD, msgClient->fd, &event);
        }
        else
        {
            printf("something else happened\n");
        }
    }
}

int main(int argc, char* argv[])
{
    if (argc <= 2)
    {
        printf("usage: %s ip_address port_number\n", basename(argv[0]));
        return 1;
    }

    const char* ip = argv[1];
    int port = atoi(argv[2]);

    int ret = 0;
    struct sockaddr_in address;
    bzero(&address, sizeof(address));
    address.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &address.sin_addr);
    address.sin_port = htons(port);

    int listenfd = socket(PF_INET, SOCK_STREAM, 0);
    assert(listenfd >= 0);

    ret = bind(listenfd, (struct sockaddr*)&address, sizeof(address));
    assert(ret != -1);

    ret = listen(listenfd, 1024);
    assert(ret != -1);

    int epollfd = epoll_create(5);
    assert(epollfd != -1);
    addfd(epollfd, listenfd);

    while (1)
    {
        int numEvents = epoll_wait(epollfd, &(*g_revents.begin())
            , static_cast<int>(g_revents.size()), -1);
        // static_cast/dynamic_cast/interpret_cast/const_cast

        if (numEvents > 0)
        {
            if (numEvents == g_revents.size())
            {
                g_revents.resize(g_revents.size() * 2);
            }
            handle_event(&(*g_revents.begin()), numEvents, epollfd, listenfd);
        }
    }

    close(listenfd);

    return 0;
}
