#ifndef EPOLL_UTILS_H
#define EPOLL_UTILS_H

void handle_new_connection(int epfd, int listen_fd);
void handle_client_data(int epfd, int client_fd, int event_fd);

#endif