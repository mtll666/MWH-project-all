#ifndef THREAD_POOL_HPP
#define THREAD_POOL_HPP

#include <queue>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

// 线程池类，异步处理FFmpeg任务
class ThreadPool {
public:
    // 构造函数，初始化指定数量的线程
    ThreadPool(size_t num_threads);

    // 析构函数，停止线程池并清理
    ~ThreadPool();

    // 添加任务到线程池
    void enqueue(std::function<void()> task);

private:
    std::vector<std::thread> workers; // 工作线程
    std::queue<std::function<void()>> tasks; // 任务队列
    std::mutex queue_mutex; // 任务队列互斥锁
    std::condition_variable condition; // 条件变量
    std::atomic<bool> stop; // 停止标志
};

#endif // THREAD_POOL_HPP