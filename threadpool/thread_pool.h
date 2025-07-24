#ifndef THREAD_POOL_H
#define THREAD_POOL_H

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
	std::atomic<bool> stop; //标志线程池是否关闭 原子变量
	
	void worker();//每个线程执行的函数
};

#endif