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
	stop.store(true); //通知线程停止
	condition.notify_all(); //唤醒所有线程
	
	for(std::thread &t : workers){
		if(t.joinable()) t.join(); //等线程退出
	}
}
//退出前将stop设置为true
//所有线程被 notify_all() 唤醒（即使队列是空的）
//主线程等待所有工作线程退出

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

