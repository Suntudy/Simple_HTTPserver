cmake_minimum_required(VERSION 3.10)
project(MyHttpServer)

set(CMAKE_CXX_STANDARD 17)

include_directories(
    ${CMAKE_SOURCE_DIR}/server
    ${CMAKE_SOURCE_DIR}/threadpool
)

# 添加你的源文件
add_executable(server
    main.cpp
    server/socket_utils.cpp
    server/epoll_utils.cpp
    threadpool/thread_pool.cpp
)
