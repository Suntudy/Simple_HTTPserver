#include "socket_utils.h"
#include "epoll_utils.h"
#include "thread_pool.h"

#include <sys/epoll.h>
#include <unistd.h>
#include <fcntl.h>
#include <iostream>
#include <sys/eventfd.h>


int main() {
    std::cout << "Starting server..." << std::endl;

    int listen_fd = create_and_bind(8080);
    std::cout << "Created socket, listen_fd = " << listen_fd << std::endl;

    set_non_blocking(listen_fd);
    std::cout << "Set non-blocking." << std::endl;

    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        return 1;
    }

    int event_fd = eventfd(0, EFD_NONBLOCK);

    epoll_event ev;
    ev.data.fd = listen_fd;
    ev.events = EPOLLIN | EPOLLET;
    epoll_ctl(epfd, EPOLL_CTL_ADD, event_fd, &ev);

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev) == -1) {
        perror("epoll_ctl");
        return 1;
    }

    ThreadPool pool(4); //4个线程

    std::cout << "Listening on port 8080..." << std::endl;

    epoll_event events[1024];
    while (true) {
        std::cout << "Waiting for epoll events..." << std::endl;
        int n = epoll_wait(epfd, events, 1024, -1);
        std::cout << "epoll_wait returned " << n << " events." << std::endl;

        for (int i = 0; i < n; ++i) {
            int fd = events[i].data.fd;
            if (fd == listen_fd) {
                std::cout << "New client connection..." << std::endl;
                handle_new_connection(epfd, listen_fd);
            } 
            else if(fd == event_fd)
            {
                //eventfd 被写入，读取一下清除信号
                uint64_t u;
                read(event_fd, &u, sizeof(u));
                //TODO: 处理需要唤醒主线程做的事，比如：清理连接、修改epoll等。
            }
            else {
                std::cout << "Data from client fd: " << fd << std::endl;
                pool.enqueue([epfd,fd,event_fd]() {
                handle_client_data(epfd, fd, event_fd);
                });
            }
        }
    }
}
