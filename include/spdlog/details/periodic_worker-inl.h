// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef SPDLOG_HEADER_ONLY
#    include <spdlog/details/periodic_worker.h>
#endif

namespace spdlog {
namespace details {

SPDLOG_INLINE periodic_worker::periodic_worker(const std::function<void()> &callback_fun, std::chrono::seconds interval)
{
    // interval不合法
    active_ = (interval > std::chrono::seconds::zero());
    if (!active_)
    {
        return;
    }

    // 线程在构建完毕后就会开始运行
    worker_thread_ = std::thread([this, callback_fun, interval]() {
        for (;;)
        {
            std::unique_lock<std::mutex> lock(this->mutex_);
            /*
                wait_for(锁，时间周期，pred谓词判断)
                返回值为pred的返回值。
                只有当 pred 条件为 false 时调用 wait_for() 才会阻塞当前线程。
                被其他线程唤醒或到时后，调用pred，返回pred的返回值
            */
            if (this->cv_.wait_for(lock, interval, [this] { return !this->active_; }))
            {
                return; // active_ == false, so exit this thread
            }
            callback_fun();
        }
    });
}

// stop the worker thread and join it
SPDLOG_INLINE periodic_worker::~periodic_worker()
{
    if (worker_thread_.joinable())
    {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            active_ = false;
        }
        cv_.notify_one(); // 这个是主线程调用的
        worker_thread_.join();
    }
}

} // namespace details
} // namespace spdlog
