// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

// multi producer-multi consumer blocking queue.
// enqueue(..) - will block until room found to put the new message.
// enqueue_nowait(..) - will return immediately with false if no room left in
// the queue.
// dequeue_for(..) - will block until the queue is not empty or timeout have
// passed.

#include <spdlog/details/circular_q.h>

#include <condition_variable>
#include <mutex>

namespace spdlog {
namespace details {

template<typename T>
class mpmc_blocking_queue
{
public:
    using item_type = T;
    explicit mpmc_blocking_queue(size_t max_items)
        : q_(max_items)
    {}

#ifndef __MINGW32__
    // try to enqueue and block if no room left
    void enqueue(T &&item)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            pop_cv_.wait(lock, [this] { return !this->q_.full(); });
            q_.push_back(std::move(item));
        }
        push_cv_.notify_one();
    }

    // enqueue immediately. overrun oldest message in the queue if no room left.
    void enqueue_nowait(T &&item)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            q_.push_back(std::move(item));
        }
        push_cv_.notify_one();
    }

    // try to dequeue item. if no item found. wait up to timeout and try again
    // Return true, if succeeded dequeue item, false otherwise
    bool dequeue_for(T &popped_item, std::chrono::milliseconds wait_duration)
    {
        {
            std::unique_lock<std::mutex> lock(queue_mutex_);
            if (!push_cv_.wait_for(lock, wait_duration, [this] { return !this->q_.empty(); }))
            {
                return false;
            }
            popped_item = std::move(q_.front());
            q_.pop_front();
        }
        pop_cv_.notify_one();
        return true;
    }

#else
    // apparently mingw deadlocks if the mutex is released before cv.notify_one(),
    // so release the mutex at the very end each function.

    // try to enqueue and block if no room left
    void enqueue(T &&item)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_); // 这样构造的unique_lock默认会上锁，无法获得锁时会阻塞。
                                                         // 销毁时会主动调用unlock()解锁
        /*
            参考: https://blog.csdn.net/qq_38231713/article/details/106092714
            wait(锁，pred谓词判断)
            pred返回值是false，那么wait()将【解锁】互斥量，并阻塞到本行，等待其它线程唤醒;
            如果pred返回值是true，那么wait()直接返回并继续执行

            当有线程唤醒正在wait的线程后：
                1、wait()不断尝试获取互斥量锁，获取不到那么流程就卡在这里；获取到了就继续执行
                2、判断pred
                    a)如果pred为false，那wait又对互斥量【解锁】，然后又休眠，等待再次被notify_one()唤醒
                    b)如果pred为true，则wait返回，流程可以继续执行（此时互斥量已被锁住）。
        */
        pop_cv_.wait(lock, [this] { return !this->q_.full(); }); // 等待队列有位置。这里用的cv是pop，是因为在后面的代码中，
                                                                 // pop_front之后调用的是pop_cv_的notify_one(pop_front后队列便有空位了)
        q_.push_back(std::move(item));
        push_cv_.notify_one(); // push_back后非空，唤醒因为队空而阻塞在push_cv_上的线程
    }

    // enqueue immediately. overrun oldest message in the queue if no room left.
    void enqueue_nowait(T &&item)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_); // 销毁时会主动调用unlock()解锁
        q_.push_back(std::move(item));
        push_cv_.notify_one();
    }

    // try to dequeue item. if no item found. wait up to timeout and try again
    // Return true, if succeeded dequeue item, false otherwise
    bool dequeue_for(T &popped_item, std::chrono::milliseconds wait_duration)
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        if (!push_cv_.wait_for(lock, wait_duration, [this] { return !this->q_.empty(); }))
        {
            return false;
        }
        popped_item = std::move(q_.front());
        q_.pop_front();
        pop_cv_.notify_one(); // pop_front后非满，唤醒因为队满而阻塞在pop_cv_上的线程
        return true;
    }

#endif

    size_t overrun_counter()
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return q_.overrun_counter();
    }

    size_t size()
    {
        std::unique_lock<std::mutex> lock(queue_mutex_);
        return q_.size();
    }

private:
    std::mutex queue_mutex_;
    std::condition_variable push_cv_;
    std::condition_variable pop_cv_;
    spdlog::details::circular_q<T> q_;
};
} // namespace details
} // namespace spdlog
