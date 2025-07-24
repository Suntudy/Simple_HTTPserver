# 设计框架
## 多线程模型（线程池）
主线程：负责接受客户端连接并将连接分发给IO线程
IO线程：负责网络事件（读/写）处理，与epoll配合
业务线程：负责业务逻辑处理，如请求解析、响应生成

## 跨线程唤醒机制
比如使用 eventfd 或管道+epoll 机制，支持IO线程唤醒业务线程等，实现高效线程通信。

## 线程安全设计
对共享数据结构使用 std::atomic (C++11) 减少锁竞争，提升性能。

# 核心技术
## 网络通信（Linux）
1. 使用 epoll 实现 I/O 多路复用：边缘触发(ET)+非阻塞模式，减少无效系统调用，提高并发性能。

2. Socket 封装：将 socket 的创建、绑定、监听、accept、send、recv 等操作封装成类，提高可读性与复用性。

## HTTP 协议支持
基本支持：解析 HTTP请求报文（支持GET/POST）；构造HTTP响应报文。
可扩展：MIME类型支持；状态码管理、keep-alive 长连接管理

## 并发处理
线程池机制：主线程将任务提交到任务队列，由业务线程从队列中取任务处理。
锁竞争优化：尽可能减少临界区，使用 std::atomic 替代 mutex 的场景，如连接计数、状态标识位等。

## 高性能优化点
零拷贝（如 sendfile）
内存池/对象池
定时器（处理连接超时）

# 项目结构
```C++
http_server/
│
├── main.cpp                // 程序入口
├── Server.h / Server.cpp   // 核心服务器类，主控制逻辑
├── ThreadPool.h            // 线程池实现
├── EventLoop.h             // IO 事件循环（封装 epoll）
├── Channel.h               // 对应一个 socket fd 的事件
├── Socket.h                // socket 封装类
├── HttpRequest.h           // 请求解析
├── HttpResponse.h          // 响应生成
├── Connection.h            // 表示一个客户端连接
├── Timer.h (可选)          // 超时连接管理
└── util/                   // 工具函数，如 MIME 类型表，日志工具等
```

# 编译运行
使用 CMake 管理构建
使用 g++ -std=c++11 -pthread 开启多线程支持

# 新技术点——作用
C++11 新特性——原子变量、智能指针、线程
Linux 网络编程——socket、epoll、非阻塞IO
多线程——线程池、任务调度、跨线程通信
HTTP 协议——请求解析、响应生成
高性能优化——ET模式、原子操作、封装思想

# 相关概念
socket——电话插座，连接客户端
epoll——智能闹钟，谁来了叫醒我
主线程——门口迎客的人
IO线程——负责点菜的服务员（收发数据）
业务线程——厨师，负责处理业务（返回网页）
线程池——提前准备好的多名厨师
原子变量——安全笔，多人写字不冲突
封装——把复杂用法包成类，方便复用

# 安装linux-ubuntu
## 安装开发工具
在ubuntu终端安装开发服务器项目所需的基础工具。
```
sudo apt update
sudo apt install -y build-essential cmake gdb git curl
```
g++——C++ 编译器
make / cmake——构建工具
gdb——调试工具
git——克隆项目 / 版本控制
curl——可用于测试 HTTP 请求


# linux命令
新建文件夹：mkdir 文件夹名
新建文件：touch 文件名 或 > 文件名
删除文件夹：rm -r 文件夹名


# 写一个简单的socket服务端程序
目标：在linux下写一个C++程序，它能对客户端（比如浏览器）发送的请求做出响应。
## 步骤
1. 创建服务器套接字（socket），建立通信的基础——socket()
2. 设置地址（IP+端口），告诉操作系统我们在哪监听——bind()
3. 开始监听，准备接收客户端连接——listen()
4. 接受连接，有客户端连进来就处理——accept()
5. 处理请求，读取请求数据，生成回应——recv()+send()
6. 关闭连接，清理资源——close()

```C++
#include <iostream>     // 打印信息到终端
#include <unistd.h>     // close() 函数需要
#include <netinet/in.h> // sockaddr_in 和相关网络结构体
#include <cstring>      // 使用 memset 或 strlen

int main()
{
	int server_fd = socket(AF_INET,SOCK_STREAM,0);
    //创建一个服务器套接字，用于后续监听客户端连接
    //AF_INET：使用IPv4
    //SOCK_STREAM：使用TCP（面向连接）
    //0:默认协议（TCP）
    
    //设置服务器的IP地址和端口
    sockaddr_in address{};//sockaddr_in是Linux下定义的“网络地址”结构体。
    address.sin_family=AF_INET;//IPv4
    address.sin_addr.s_addr=INADDR_ANY;//任意网卡(0.0.0.0)，监听所有IP
    address.sinport=htons(8080);//设置端口号为8080，htons是字节序转换
    bind(server_fd, (sockaddr*)&address, sizeof(address));//把socket和设置的地址/端口绑定起来，相当于“注册”到操作系统上。
    listen(server_fd, 10);// 最多允许10个等待连接的客户端
    
     std::cout << "Server listening on port 8080..." << std::endl;
     
     while(true)
     {
     	int client_fd = accept(server_fd,nullptr,nullptr);
     	//nullptr,nullptr两个参数，不关心客户端的IP，只要能连就行。
     	
     	const char* response = //最基础的HTTP响应，返回一个helloworld字符串。
     		"HTTP/1.1 200 OK\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: 13\r\n"
            "\r\n"
            "Hello, world!";
            
        send(client_fd,response,strlen(response),0);
        close(client_fd);
     }
     
     close(server_fd);
     return 0;
}
```
# Linux TCP服务端创建流程
1. socket()：创建一个套接字
2. bind()：将套接字绑定到本地IP和端口上
3. listen()：监听这个端口，准备接受连接
4. accept()：接收客户端连接
5. read()/recv()：接收数据
6. write()/send()：发送数据
7. close()：关闭连接
## socket() 套接字
创建一个“网络通信用的文件描述符”
```C++
int socket(int domain, int type, int protocol);

domain:协议族（决定用什么网络），常用值：AF__INET（IPv4）
type:套接字类型（通信方式），常用值：SOCK_STREAM（TCP）或 SOCK_DGRAM（UDP）
protocol:协议编号（通常为0：系统根据上面自动推导）


//例子
int listen_fd=socket(AF_INET, SOCK_STREAM,0);
//创建一个用于TCP通信的IPv4套接字
```
## bind()
告诉操作系统你要占住哪个IP和端口
把刚刚创建的socket绑定到本地IP地址和端口号上，告诉操作系统：我要在这个端口监听连接。

如果不调用bind()，这个socket虽然存在，但是操作系统不知道它要在哪“听”。
```C++
int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);

sockfd: 前面用socket()得到的文件描述符
addr：要绑定的地址结构体（包括IP和端口）
addrlen：地址结构体的长度（通常用sizeof()得到）

//例子
#include<arpa/inet.h> //inet_pton
#include<sys/socket.h>
#include<netinet/in.h> //sockaddr_in
#include<cstring> //memset

struct sockaddr_in addr;
memset(&addr,0,sizeof(addr));
addr.sin_family = AF_INET; //IPv4
addr.sin_port = htons(8080); //端口号（注意要转成网络字节序）
addr.sin_addr.s_addr = INADDR_ANY; //监听所有本机网卡的IP

bind(sockfd,(struct sockaddr*)&addr,sizeof(addr));

//端口号：定位到具体程序
//IP地址只能定位到“哪台机器”，端口号才能定位到“机器里的哪个程序”。
```
## listen()
开始监听，准备接收连接请求
在socket(),bind()之后，listen()是告诉操作系统：这个socket已经绑定好IP和端口了，现在开始监听，欢迎客户端来连接我。
```C++
int listen(int sockfd, int backlog);

//sockfd: 前面通过socket和bind过的socket描述符
//backlog: 等待连接的“排队队列”长度（最大连接等待数）

if(listen(sockfd,128) == -1)
{
	perror("listen failed");
	close(sockfd);
	return 1;
}
//服务器最多能同时排队等候128个客户端的连接请求，一旦超过，操作系统就可能拒绝新连接或返回错误。
//注意：这不是并发连接的上限，而是“等待accept的连接”数量上限。
```
listen() 只对**面向连接的socket（TCP）**有意义，UDP（SOCK_DGRAM）不需要。
一般写的HTTP服务器，用的是TCP，所以一定需要调用 listen()
如果不listen就直接accept，程序会直接出错。

## accept()
接收客户端连接
在调用listen()之后，操作系统会帮你把客户端的连接请求排队。
接下来需要用 accept() 从队列里“取出一个连接”，拿到和客户端通信的新socket。
```C++
int accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);

sockfd: 之前listen的socket
addr: 输出参数：客户端的地址信息（可以为nullptr）
addrlen: 输入/输出参数：地址结构体的大小（可以为nullptr）

//例子
struct sockaddr_in client_addr;//定义了一个空变量
socklen_t client_len=sizeof(client_addr);
int conn_fd=accept(listen_fd, (struct sockaddr*)&client_addr, &client_len);
//accept() 自动填进去客户端IP

listen_fd：是你监听的socket
conn_fd：是新的连接socket，用它和这个客户端通信
client_addr 保存了客户端的IP、端口信息（可选）
每次accept()成功，就得到了一个新socket，用它来和这个特定客户端通信。

```
## read(), write()
和客户端收发数据
read()：从客户端接收数据（读取对方发来的内容）
write()：发送数据给客户端（如返回HTML网页）
两个函数的调用对象都是accept()返回的新socket
```C++
ssize_t read(int fd, void *buf, size_t count);
ssize_t write(int fd, const void *buf, size_t count);

fd: accept()的新socket（conn_fd）
buf: 数据缓冲区
count: 缓冲区大小
返回值: 实际读/写的字节数，出错返回-1

//read 例子
char buffer[1024]={0};
int bytes_read = read(conn_fd, buffer, sizeof(buffer)-1);
if(bytes_read > 0)
{
	buffer[bytes_read]='\0';//确保是字符串结尾
	std::cout<<"客户端发来的内容："<<buffer<<std::endl;
}

//write 例子：写一个最简单的HTTP响应
const char *response =
	"HTTP/1.1 200 OK\r\n"          //响应行
	"Content-Type: text/html\r\n"  //告诉浏览器内容类型是HTML
	"Content-Length: 13\r\n"       //正文的字节数
	"\r\n"                         //分隔头部和正文
	"Hello, world!";               //实际的网页内容
write(conn_fd, response, strlen(response));

//响应行："HTTP/1.1 200 OK\r\n"
//向客户端（通常是浏览器）说明：我是谁（版本协议），请求处理结果如何（状态码），简要说明。
//HTTP/1.1 版本协议：表示这是用HTTP/1.1协议来回应的
//200 状态码：表示请求成功
//OK 状态描述：对200的文字解释
//状态码：
//200 OK
//404 Not Found （你请求的资源不存在）
//500 Internal Error （服务器内部出错）
//301 Moved Parmanently （永久重定向，比如换网址了）
```
read() 会阻塞，直到客户端发来数据或断开
如果客户端断开，read()返回0，表示对方关闭连接
write() 可能不会一次写完所有数据（大数据时），这时要用循环写
## close()

# 多线程
**主线程 + IO线程 + 业务线程**
上述简单服务器一次只能处理一个客户端，其他人都得等，很慢。
加入线程池后，可以让服务器同时处理多个客户端连接，互不影响。

主线程：
> 负责监听socket，accept新连接

IO线程：
> 负责监听多个客户端socket的读写事件 （用epoll）

业务线程池：
> 真正处理业务逻辑，比如解析HTTP、构造响应

## 流程
1. 主线程
> 创建监听socket
> 执行 bind(), listen()
> 不直接处理连接，只是accept()并把新连接socket(conn_fd)交给IO线程

2. IO线程（用epoll实现）
> 维护一个epoll实例
> 把conn_fd注册进epoll
> 一旦某个socket可读，就读出数据，并将任务扔进线程池

3. 线程池（业务线程）
> 拿到客户端请求数据
> 解析HTTP请求
> 构造响应
> 使用 write() 返回结果给客户端

总结：
1. 主线程：accept() 得到 conn_fd，交给 IO 线程
2. IO 线程：epoll_wait() 检测到 conn_fd 可读，调用 read() 接收数据
3. IO 线程：将请求内容打包成任务，扔进线程池
4. 业务线程：处理业务逻辑，生成响应
5. 业务线程：通过 write() 把响应发回客户端

项目构成：
```C++
server.cpp           //主线程：监听、accept
epoll_wrapper.h/.cpp //IO线程 + epoll封装
threadpool.h/.cpp    //业务线程池
http_handler.h/.cpp  //处理HTTP请求
```
# epoll
read()/write()/accept() 这些函数默认是阻塞式的，一旦read，就会卡在那里等数据，不干别的。因此需要更高级的 IO模型——epoll。
epoll 是 Linux 提供的高性能IO事件通知机制，用于监听多个文件描述符（socket）是否就绪，比如是否可以读、可以写。

## epoll和select/poll的区别
select/poll 每次都轮询所有socket，一个个查。低效，连接多时很慢
epoll 内核帮你记录哪些socket有事件发生。高效，能支持成千上万个连接。
```C++
int epoll_fd = epoll_creat(1024);//创建epoll实例

epoll_ctl(epoll_fd, EPOLL_CTL_ADD, sockfd, &event);//注册socket事件

int n = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);//等待事件发生
```
## epoll两种模式
1. LT (Level Trigger, 默认) 只要还有数据没读，每次 epoll_wait() 都会通知你。
2. ET (Edge Trigger) 只在状态变化时通知一次（数据来了瞬间）
高性能服务器常采用 ET+非阻塞IO 模式。
## epoll 特性
1. epoll 是事件驱动的，它不是一个个检查socket，而是等事件来通知。
2. 不会阻塞主线程，你可以一次等多个连接，谁有数据来，谁就处理。
3. 搭配非阻塞 socket 和 ET模式 更高效，不再重复处理已读干净的数据。
4. 支持成千上万连接，比select高效的多。
```C++
int server_fd = socket(...);         // 创建监听 socket（用于 accept）
bind(server_fd, ...);
listen(server_fd, ...);

int epfd = epoll_create(1);         // 创建 epoll 实例

// ✅ 注册监听 socket，监听“有没有新连接”
struct epoll_event ev;
ev.events = EPOLLIN;
ev.data.fd = server_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

while (true) {
    // ✅ 等待“事件发生”的 socket
    int n = epoll_wait(epfd, events, 1024, -1);

    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;

        // 👉 是监听 socket 的事件，说明有新连接
        //客户端的 socket 并不是直接发给 server_fd 的，而是向 server_fd 所绑定的 IP 和端口请求连接。操作系统内核会检测到这个连接请求，把 server_fd 标记为“就绪”（可读），然后你就可以 accept() 得到一个新的 conn_fd。
        if (fd == server_fd) {
            int conn_fd = accept(server_fd, ...);

            // ✅ 把 conn_fd 也注册到 epoll，关注“这个客户端有没有数据来”
            struct epoll_event client_ev;
            client_ev.events = EPOLLIN;
            client_ev.data.fd = conn_fd;
            epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &client_ev);
        }
        // 👉 是客户端 socket 可读
        else {
            char buf[1024];
            int len = read(fd, buf, sizeof(buf));
            if (len > 0) {
                // 处理客户端数据
            } else {
                // 断开连接
                close(fd);
                epoll_ctl(epfd, EPOLL_CTL_DEL, fd, nullptr);
            }
        }
    }
}
```
## ET + 非阻塞IO
ET 模式下：你只会被通知 一次：“刚才变成可读了”，之后除非新数据又来了，否则就不会再提醒。

ET 必须配合非阻塞IO，因为你只有一次机会被通知，必须一次性把所有数据都读干净。

阻塞IO：如果你调用 read()，但数据没准备好，程序会卡住不动
非阻塞IO：如果你调用 read()，但没数据，直接返回 -1，不会阻塞程序

ET 模式下一定要使用非阻塞 IO，否则可能永远读不到新数据了。
```C++
//设置 非阻塞IO
int flags = fcntl(fd, F_GETFL, 0);
fcntl(fd, F_SETFL, flags | O_NONBLOCK);  // 设置为非阻塞
```
原因	解释
ET 只通知一次	你必须一次性处理完所有数据
阻塞 IO 会卡住	没数据时会卡住程序，错过后续事件
非阻塞 IO 能快速失败	你可以循环读取直到读完或返回 EAGAIN
否则程序就会死在 read()	再也收不到 epoll 的通知，导致死锁
```C++
#include <fcntl.h>  // 头文件一定要加！

// 设置 conn_fd 为非阻塞
int flags = fcntl(conn_fd, F_GETFL, 0);
fcntl(conn_fd, F_SETFL, flags | O_NONBLOCK);

//注册事件时，添加EPOLLET（边缘触发）
epoll_event ev;
ev.events = EPOLLIN | EPOLLET;  // ET 模式！
ev.data.fd = conn_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, conn_fd, &ev);

//处理客户端数据时，一定要 while(read)读干净
char buf[1024];
while (true) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) {
        // 处理 buf 中的数据
        write(fd, buf, n);  // 简单回显
    } else if (n == 0) {
        // 客户端关闭连接
        close(fd);
        break;
    } else {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 所有数据都读完了
            break;
        } else {
            perror("read");
            close(fd);
            break;
        }
    }
}

//server_fd 也可以设置为 非阻塞+EPOLLET
//注意accept() 需要while循环，多次调用直到没有新连接位置。（返回EAGAIN为止）
int flags = fcntl(server_fd, F_GETFL, 0);
fcntl(server_fd, F_SETFL, flags | O_NONBLOCK);

ev.events = EPOLLIN | EPOLLET;
ev.data.fd = server_fd;
epoll_ctl(epfd, EPOLL_CTL_ADD, server_fd, &ev);

while (true) {
    int conn_fd = accept(server_fd, nullptr, nullptr);
    if (conn_fd == -1) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // 没有更多连接了
            break;
        } else {
            perror("accept");
            break;
        }
    }

    // 设置 conn_fd 为非阻塞，并注册进 epoll
}
```
# 封装
## 封装 Socket
int create_and_bind(int port);
int set_non_blocking(int fd);

### create_and_bind(int port)
这个函数完成了服务器创建监听socket的全部准备工作。
1. 创建socket
2. 设置端口复用
3. 绑定IP和端口
4. 启动监听(listen)

```C++
//socket_utils.h/.cpp

#include<sys/socket.h>
#include<netinet/in.h>
#include<cstring>
#include<unistd.h>
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

//调用时，只需要一句话，就可以得到一个监听客户端连接的socket文件描述符(listen_fd)
int listen_fd = create_and_bind(8080);
```
### set_non_blocking(int fd)
把一个文件描述符(比如socket)设置成“非阻塞”模式，配合 epoll + ET使用。
```C++
//socket_utils.h/.cpp

#include<fcntl.h>

int set_non_blocking(int fd)
{
	//读取原来的文件状态标志位
    int flags = fcntl(fd, F_GETFL, 0);
    
	if(flags == -1) return -1;
	return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
	//加上 O_NONBLOCK，设置为“非阻塞”
}

//调用
int listen_fd = create_and_bind(8080);
set_non_blocking(listen_fd);
//listen_fd就变成了一个准备就绪、非阻塞的监听socket
```

## 封装epoll
void handle_new_connection(int epfd, int listen_fd);
void handle_client_data(int epfd, int client_fd);

### handle_new_connection(int epfd, int listen_fd)
接收新的连接，并注册到epoll
1. accept()循环接收所有新连接（ET模式必须用while）
2. 设置新连接为非阻塞
3. 注册到epoll中监听 EPOLLIN ｜ EPOLLET（读事件+边缘触发）

### handle_client_data(int epfd, int client_fd)
接收客户端数据，进行处理（比如读取请求&回复响应）
1. read() 循环读取所有数据（ET模式必须用while）
2. 出错：关闭连接+从epoll删除
3. 收到数据：直接构造HTTP响应并write()回去

```C++
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

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

        std::cout << "New client connected, fd: " << conn_fd << std::endl;
    }
}

void handle_client_data(int epfd, int client_fd) {
    char buffer[4096];
    while (true) {
        ssize_t n = read(client_fd, buffer, sizeof(buffer));
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("read");
            close(client_fd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, nullptr);
            return;
        } else if (n == 0) {
            std::cout << "Client disconnected: " << client_fd << std::endl;
            close(client_fd);
            epoll_ctl(epfd, EPOLL_CTL_DEL, client_fd, nullptr);
            return;
        }

        // 回显响应（简单模拟 HTTP）
        std::string response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, World!";
        write(client_fd, response.c_str(), response.size());
    }
}
```

## 封装后的代码 main
```C++
//main.cpp
int listen_fd = create_and_bind(8080);
set_non_blocking(listen_fd);

int epfd = epoll_create1(0);

epoll_event ev;
ev.data.fd = listen_fd;
ev.events = EPOLLIN | EPOLLET;
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

epoll_event events[1024];

while (true) {
    int n = epoll_wait(epfd, events, 1024, -1);
    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;
        if (fd == listen_fd) {
            handle_new_connection(epfd, listen_fd);
        } else {
            handle_client_data(epfd, fd);//⚠️在主线程中处理
        }
    }
}
```
# 线程池
运行流程：
epoll_wait——事件来了：
1. 新连接 accept
2. 有数据要处理（read）——主线程把任务放进任务队列——某个线程被唤醒，取出任务处理——执行业务逻辑（回写数据等）

线程池的好处：
1. 主线程不阻塞：主线程只负责接收请求
2. 高并发处理：多个线程一起跑，多个客户端同时服务
3. 线程复用：避免频繁创建/销毁线程，节省性能
4. 更好地控制线程数量：防止开太多线程耗尽资源

## 创建 ThreadPool 类
```C++
//thread_pool.h

#ifndef THREAD_POOL_H
#defind THREAD_POOL_H

#include<vector>
#include<queue>
#include<thread>
#include<mutex>
#include<condition_variable>
#include<functional>
#include<atomic>

class ThreadPool {
public:
	ThreadPool(size_t num_threads);//构造函数：初始化线程池
	~ThreadPool();//析构函数：清理线程资源
	
	void enqueue(std::function<void()>task);//添加任务
	
private:
	std::vector<std::thread> workers;//所有工作线程
	std::queue<std::function<void()>> tasks;//等待执行的任务队列
	
	std::mutex queue_mutex; //互斥锁保护队列
	std::condition_variable condition; //条件变量实现阻塞等待
	std::atomic<bool> stop; //标志线程池是否关闭
	
	void worker();//每个线程执行的函数
}

#endif
```
```C++
//thread_pool.cpp

#include "thread_pool.h"

//构造函数
ThreadPool::ThreadPool(size_t num_threads) : stop(false){
	for(size_t i=0; i<num_threads; ++i)
	{
		workers.emplace_back([this]() {this->worker();});
	}
}
//stop(false):先设置线程池运行状态为“未停止”
//emplace_back():每创建一个线程，就用worker()函数
//最终线程池会启动num_threads个线程，它们会一直在后台等待任务执行


//析构函数
ThreadPool::~ThreadPool(){
	stop=true; //通知线程停止
	condition.notify_all(); //唤醒所有线程
	
	for(std::thread &t : workers){
		if(t.joinable()) t.join(); //等线程退出
	}
}
//退出前将stop设置为true
//所有线程被 notify_all() 唤醒（即使队列是空的）
主线程等待所有工作线程退出


//添加任务 enqueue()
void ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex);//加锁
        tasks.push(std::move(task));//添加任务
    }
    condition.notify_one(); // 唤醒一个等待的线程
}
//把任务塞进 tasks 队列中
//然后 notify_one() 通知一个线程来执行这个任务


//线程真正执行的函数 worker()
void ThreadPool::worker() {
    while (!stop) {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            condition.wait(lock, [this]() {
                return stop || !tasks.empty();
            });

            if (stop && tasks.empty())
                return;

            task = std::move(tasks.front());
            tasks.pop();
        }
        task(); // 执行任务
    }
}
//一直循环等待任务
//condition.wait()会阻塞，直到：有新任务(!tasks.empty())，或线程池关闭(stop==true)
//如果任务队列非空，就取出任务，执行它task()

```
```C++
//main.cpp

#include "thread_pool.h"

int main() {
    ThreadPool pool(4);  // 开 4 个线程

    // 添加一些测试任务
    for (int i = 0; i < 10; ++i) {
        pool.enqueue([i]() {
            printf("任务 %d 在线程 %ld 中运行\n", i, std::this_thread::get_id());
        });
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}
```
## 线程池-服务器代码
把处理客户端请求的逻辑(handle_client_data())交给线程池执行，而不是主线程自己干活。
```C++
//main.cpp
int listen_fd = create_and_bind(8080);
set_non_blocking(listen_fd);

int epfd = epoll_create1(0);

epoll_event ev;
ev.data.fd = listen_fd;
ev.events = EPOLLIN | EPOLLET;
epoll_ctl(epfd, EPOLL_CTL_ADD, listen_fd, &ev);

epoll_event events[1024];

ThreadPool pool(4);

while (true) {
    int n = epoll_wait(epfd, events, 1024, -1);
    for (int i = 0; i < n; ++i) {
        int fd = events[i].data.fd;
        if (fd == listen_fd) {
            handle_new_connection(epfd, listen_fd);
        } 
        else 
        {//⚠️在线程池中处理任务
            pool.enqueue([epfd,fd]() {
			handle_client_data(epfd,fd);
			});
			//epfd和fd是值传递，可以在线程里安全使用
			//handle_client_data()内部的代码要确保是线程安全的（比如不会两个线程同时对同一个 socket 写数据）。
        }
    }
}
```
# 接下来
1. 增强服务功能
✅ 可选增强功能：
功能	描述
✅ 支持简单的 HTTP 请求解析	解析 GET 请求的路径等
✅ 静态文件返回功能	GET /index.html 返回 HTML 文件
响应真实 HTTP 请求	解析请求行/头，返回不同网页内容
HTTP 协议支持
基本支持：解析 HTTP请求报文（支持GET/POST）；构造HTTP响应报文。
可扩展：MIME类型支持；状态码管理、keep-alive 长连接管理
✅ 日志系统	输出连接日志（加锁写文件）
✅ 超时连接关闭	设置定时器清理长时间不用的连接
✅ 增加信号处理	捕捉 Ctrl+C、安全退出，释放资源
2. 支持CMake编译
3. 对共享数据结构使用 std::atomic (C++11) 减少锁竞争，提升性能。
锁竞争优化：尽可能减少临界区，使用 std::atomic 替代 mutex 的场景，如连接计数、状态标识位等。
4. 实现跨线程唤醒

# 增强服务器响应功能
## 返回静态HTML文件
一个典型的 HTTP 请求长这样：
```C++
GET /index.html HTTP/1.1
Host: localhost:8080
User-Agent: curl/7.81.0
Accept: */*
//主要解析第一行（请求行）：
//方法：GET
//路径：/index.html
//协议版本：HTTP/1.1
```
```C++
void handle_client_data(int epfd, int client_fd) {
	//读取 socket 内容（字符串格式）
	char buffer[4096];
	int n=read(fd,buffer,sizeof(buffer)-1);
	if(n<=0) {close(fd);return;}
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
		write(fd,not_found.c_str(),not_found.size());
		close(fd);
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
	
	write(fd,response.c_str(),response.size());
	close(fd);//简易实现，短连接
}
```
### HTML
HTML 是一种**超文本标记语言**，用于构建网页内容。我们在浏览器中看到的所有网页，基本上都是由HTML写出来的。
可以把 HTML 看作网页的骨架/结构，浏览器读取后，就会把这些内容渲染成网页界面。
服务器返回一个 HTML文件，浏览器拿到之后，会把它“翻译”成图形化的网页。
```C++
//一个简单的例子
<!DOCTYPE html> //声明这个文件是HTML5
<html> //整个网页的根节点
  <head> //网页的头部，放标题、编码、样式等信息
    <title>我的网页</title> //网页标题，会显示在浏览器标签上
  </head>
  <body> //网页的正文内容
    <h1>欢迎来到我的服务器！</h1> //一级标题
    <p>这是一个用 C++ 写的服务器返回的网页。</p> //段落文本
  </body>
</html>
```
## （优化）添加更多的 MIME类型支持
## （优化）将代码结构优化为模块
拆分成多个小函数
## （优化）搭配一个实际HTML网页测试效果
## 支持POST请求

# CMake编译
CMake是一个跨平台的自动化构建系统生成工具。
主要作用是：
用一种统一的方式管理、组织、构建C/C++项目
自动生成不同平台的构建文件
帮你处理复杂的依赖关系、编译选项等
如果项目结构如下：
```C++
/myserver/
├── CMakeLists.txt
├── main.cpp
├── server/
│   ├── server.cpp
│   └── server.h
├── threadpool/
│   ├── pool.cpp
│   └── pool.h
```
需要：管理多个源文件和头文件
指定include路径
编译成目标文件
链接成可执行程序
CMake 允许用一种统一、可维护的方式，表达这些构建规则。
## 配置文件
写一个 CMakeLists.txt 配置文件：
```C++
//CMakeLists.txt

cmake_minimum_required(VERSION 3.10)
project(MyWebServer)

set(CMAKE_CXX_STANDARD 11)

add_executable(server main.cpp server/server.cpp threadpool/pool.cpp)
```
然后：
```C++
mkdir build 
//在项目目录里新建一个 build目录，用来放生成的构建文件（Makefile, 临时文件，目标文件等）
//好处：源码目录和构建目录分离，不污染原始代码

cd build
//进入刚才新建的 build目录，接下来的构建过程都在这里进行

cmake ..
//用 CMake读取项目根目录（..表示上级目录）中的 CMakeLists.txt文件，然后分析项目结构、检查依赖、生成 Makefile
//只需要写一次CMakeLists.txt，CMake会自动搞定生成规则

make
//运行make命令，读取刚刚CMake生成的Makefile，根据规则编译.cpp文件，链接成可执行文件（如server）
//最终会在build目录里看到编译出来的可执行程序

./server
```
就会自动生成 Makefile
自动找到并编译所有源文件
最终生成你的服务器程序 ./server

http://192.168.101.166:8080/

# 跨线程唤醒
如果只监听 socket 事件，不需要跨线程唤醒，epoll 本身会被网络事件唤醒。
但如果想从线程池/其他线程主动唤醒主线程，则需要跨线程唤醒，推荐使用 eventfd 或 pipe.
举例场景：
1. 线程池线程处理完任务，想告诉主线程更新 epoll 状态（比如取消监听）
2. 定时器线程需要通知 epoll 的主线程超时事件到了
3. 后台线程检测某个状态变化，主动通知主线程处理
这些就必须用 eventfd，因为主线程此时根本不知道发生了啥，还卡在 epoll_wait() 上。

## 如果不用跨线程唤醒
"线程池线程处理完任务，想告诉主线程更新 epoll 状态（比如取消监听）"，如果不用跨线程唤醒，线程池线程处理完任务后，主线程啥也不知道。
在我的服务器项目里也不需要知道，因为是短连接服务器，线程池线程最后做了三件事：处理客户端请求、write、close。
我的项目的基本结构：
主线程：
> 使用 epoll_wait() 等待 socket 活动
> 有事件时（如客户端发送 HTTP 请求），通过 ThreadPool 把任务分发给工作线程处理。

线程池中的工作线程：
> 处理客户端请求（如 读取 socket，构造 HTTP 响应）
> 最后调用 write() 回写响应，并 close() 连接

close()后，客户端连接关闭，fd被内核从 epoll 自动移除，主线程不需要介入处理“收尾”工作。

如果线程处理完任务后，不关闭 socket（要支持长连接），这时主线程仍然监听这个 fd，但 socket 没有数据来，epoll_wait() 就会一直阻塞，此时如果线程要通知主线程“请取消监听”或“我更新了这个fd的状态”，主线程永远不知道。此时必须跨线程唤醒主线程。（通过 eventfd 或 pipe）


# std::atomic
多线程共享数据时必须同步，可以用std::mutex 或 std::atomic<T>
std::mutex 会带来线程上下文切换、性能抖动、死锁风险。

std::atomic<T>提供更轻量级、无锁的方式。
适用于 操作简单的数值型数据结构，如：
连接数计数器、状态标识（比如是否关闭、是否繁忙）

std::atomic<T>是一个模版类，用来保证对某个变量的访问是原子的——即多个线程同时访问时，不会发生“数据竞争”或“中间态”。

## 项目中可以用atomic的地方
1. 连接计数器
代替mutex管理当前连接数
```C++
std::atomic<int> conn_count;
```
2. 状态位（例如标识一个socket是否在处理中）
```C++
std::atomic<bool> is_processing;
```
3. 自定义任务队列或线程池内部的标志
```C++
std::atomic<bool>stop(false);
```
## 注意事项
1. 不是所有共享数据都能用atomic代替mutex，如果是复合结构（map，vector等）依然要用锁。
2. 原子操作只适合极简单的数据读写，比如加减、布尔切换等
3. 使用 load() 和 store() 方法时要注意语义一致性（一般用默认的memory_order_seq_cst就可以）