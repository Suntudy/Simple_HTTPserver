#include"socket_utils.h"
#include<sys/socket.h>
#include<netinet/in.h>
#include<cstring>
#include<unistd.h>
#include<fcntl.h>
#include<iostream>

int create_and_bind(int port)
{
	//创建一个TCP socket
	int fd = socket(AF_INET,SOCK_STREAM,0);
	if(fd == -1)
	{
        perror("socket");
        exit(1);
	}
	
	int opt=1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	//允许端口复用(SO_REUSEADDR),避免程序关闭后端口短时间内不能重启
	//防止"Address already in use"错误
	
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = INADDR_ANY;
	//设置socket地址结构体：
		//AF_INET表示IPv4
		//htons(port)把端口号转为网络字节序
		//INADDR_ANY表示监听本机所有IP
	
	//把这个socket和指定的IP+端口绑定起来
	//绑定失败通常是因为端口被占用
	if(bind(fd, (sockaddr*)&addr, sizeof(addr)) == -1)
	{
		perror("bind");
		close(fd);
		exit(1);
	}
	
	//让socket开始监听连接请求
	//SOMAXCONN是系统允许的最大等待连接队列长度
	if(listen(fd, SOMAXCONN) == -1)
	{
		perror("listen");
		close(fd);
		exit(1);
	}
	
	return fd;
}

int set_non_blocking(int fd)
{
	//读取原来的文件状态标志位
    int flags = fcntl(fd, F_GETFL, 0);
    
	if(flags == -1) return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	//加上 O_NONBLOCK，设置为“非阻塞”
}


