#include "epoll_utils.h"
#include "socket_utils.h"
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sstream>
#include <fstream>
#include <atomic>

std::atomic<int> conn_count(0);

void handle_new_connection(int epfd, int listen_fd) {
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int conn_fd = accept(listen_fd, (sockaddr*)&client_addr, &client_len);
        if (conn_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("accept");
            break;
        }

        // 设置非阻塞
        int flags = fcntl(conn_fd, F_GETFL, 0);
        fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK);

        // 注册到 epoll 中，监听读事件（ET 模式）
        epoll_event ev;
        ev.data.fd = conn_fd;
        ev.events = EPOLLIN | EPOLLET;
        epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev);

		conn_count++;

        std::cout << "New client connected, fd: " << conn_fd << ", current connections: " << conn_count.load() << std::endl;
    }
}

void handle_client_data(int epfd, int client_fd, int event_fd) {
	//读取 socket 内容（字符串格式）
	char buffer[4096];
    int n=read(client_fd,buffer,sizeof(buffer)-1);
	if(n<=0) {close(client_fd);return;}
	buffer[n]='\0';//结束符，变成C字符串
	
	//解析请求行（第一行）
	std::istringstream request_stream(buffer);
	std::string method, path, version;
	request_stream >> method >> path >> version;
	
	//打开并读取本地文件（如：./static.index.html）
	std::string file_path="./static"+path;	
	if(path=="/") file_path="./static/index.html";
	
	std::ifstream file(file_path);
	if(!file.is_open())
	{
		std::string not_found = "HTTP/1.1 404 Not Found\r\n\r\n404 Not Found";
		write(client_fd,not_found.c_str(),not_found.size());
		close(client_fd);
		return;
	}
	std::stringstream content;
	content << file.rdbuf();
	std::string body = content.str();
	
	//构造响应内容并发送
	std::string response = "HTTP/1.1 200 OK\r\n";
	response += "Content-Length: "+std::to_string(body.size())+"\r\n";
	response += "Content-Type: text/html\r\n\r\n";
	response += body;
	
	write(client_fd,response.c_str(),response.size());

	uint64_t u=1;
	write(event_fd, &u, sizeof(u));//向 eventfd 写数据，唤醒主线程

	conn_count--;

	std::cout << "Client disconnected, fd: " << client_fd << ", current connections: " << conn_count.load() << std::endl;
	
	close(client_fd);//简易实现，短连接
}
