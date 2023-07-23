// thread_pool_demo.h

#ifndef THREAD_POOL_DEMO_H
#define THREAD_POOL_DEMO_H

#include <queue>
#include <mutex>
#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <future>

template <typename T>
class TaskQueue
{
private:
    std::queue<T> queue_;
    std::mutex    queue_mutex_;

public:
    TaskQueue() {}
    ~TaskQueue() {}

    int size() {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        return queue_.size();
    }

    bool empty() {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        return queue_.empty();
    }

    void enqueue(T &t) {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        queue_.emplace(t);
    }

    bool dequeue(T &t) {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        if (queue_.empty()) return false;

        t = std::move(queue_.front());
        queue_.pop();

        return true;
    }
};

class ThreadPoolDemo
{
private:
    bool                             close_;
    TaskQueue<std::function<void()>> task_queue_;
    std::vector<std::thread>         threads_;
    std::mutex                       thread_mutex_;
    std::condition_variable          thread_condition_;

    class Worker
    {
    private:
        int            id_;
        ThreadPoolDemo *pool_; // 所属的线程池

    public:
        Worker(const int id, ThreadPoolDemo *pool)
        : id_(id)
        , pool_(pool) {}

        ~Worker() {}

        void operator()() {
            std::function<void()> func;
            bool dequeued;

            while (!pool_->close_) {
                {
                    std::unique_lock<std::mutex> lock(pool_->thread_mutex_);

                    if (pool_->task_queue_.empty()) {
                        pool_->thread_condition_.wait(lock);
                    }

                    dequeued = pool_->task_queue_.dequeue(func);
                }
                if (dequeued) func();
            }
        }
    };

public:
    ThreadPoolDemo(const unsigned int num_of_threads = std::thread::hardware_concurrency())
    : close_(false)
    , threads_(std::vector<std::thread>(num_of_threads)) {}

    ThreadPoolDemo(const ThreadPoolDemo &) = delete;
    ThreadPoolDemo(ThreadPoolDemo &&) = delete;
    ThreadPoolDemo &operator=(const ThreadPoolDemo &) = delete;
    ThreadPoolDemo &operator=(ThreadPoolDemo &&) = delete;

    ~ThreadPoolDemo() {}

    void init() {
        for (int i = 0; i < threads_.size(); ++i) {
            threads_.at(i) = std::thread(Worker(i, this));
        }
    }

    void close() {
        close_ = true;
        thread_condition_.notify_all();

        for (int i = 0; i < threads_.size(); ++i) {
            if (threads_.at(i).joinable()) {
                threads_.at(i).join();
            }
        }
    }

    template <typename F, typename ...Args>
    auto submit(F &&f, Args &&...args) -> std::future<decltype(f(args...))> {
        std::function<decltype(f(args...))()> func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        
        auto task_ptr = std::make_shared<std::packaged_task<decltype(f(args...))()>>(func);

        std::function<void()> wrapper_func = [task_ptr]() {
            (*task_ptr)();
        };
        
        task_queue_.enqueue(wrapper_func);
        thread_condition_.notify_one();

        return task_ptr->get_future();
    }
};

#endif
