// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

// Loggers registry of unique name->logger pointer
// An attempt to create a logger with an already existing name will result with spdlog_ex exception.
// If user requests a non existing logger, nullptr will be returned
// This class is thread safe

#include <spdlog/common.h>

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

namespace spdlog {
class logger;

namespace details {
class thread_pool;
class periodic_worker;

class SPDLOG_API registry
{
public:
    // logger_name ---> level_enum
    using log_levels = std::unordered_map<std::string, level::level_enum>;
    registry(const registry &) = delete;
    registry &operator=(const registry &) = delete;

    void register_logger(std::shared_ptr<logger> new_logger);
    void initialize_logger(std::shared_ptr<logger> new_logger);
    std::shared_ptr<logger> get(const std::string &logger_name);
    std::shared_ptr<logger> default_logger();

    // Return raw ptr to the default logger.
    // To be used directly by the spdlog default api (e.g. spdlog::info)
    // This make the default API faster, but cannot be used concurrently with set_default_logger().
    // e.g do not call set_default_logger() from one thread while calling spdlog::info() from another.
    logger *get_default_raw();

    // set default logger.
    // default logger is stored in default_logger_ (for faster retrieval) and in the loggers_ map.
    void set_default_logger(std::shared_ptr<logger> new_default_logger);

    void set_tp(std::shared_ptr<thread_pool> tp);

    std::shared_ptr<thread_pool> get_tp();

    // Set global formatter. Each sink in each logger will get a clone of this object
    void set_formatter(std::unique_ptr<formatter> formatter);

    void enable_backtrace(size_t n_messages);

    void disable_backtrace();

    void set_level(level::level_enum log_level);

    void flush_on(level::level_enum log_level);

    // 指定间隔时间flush一遍
    void flush_every(std::chrono::seconds interval);

    void set_error_handler(err_handler handler);

    void apply_all(const std::function<void(const std::shared_ptr<logger>)> &fun);

    void flush_all();

    void drop(const std::string &logger_name);

    void drop_all();

    // clean all resources and threads started by the registry
    void shutdown();

    std::recursive_mutex &tp_mutex();

    void set_automatic_registration(bool automatic_registration);

    // set levels for all existing/future loggers. global_level can be null if should not set.
    void set_levels(log_levels levels, level::level_enum *global_level);

    static registry &instance();

private:
    registry();
    ~registry();

    void throw_if_exists_(const std::string &logger_name);
    void register_logger_(std::shared_ptr<logger> new_logger);
    bool set_level_from_cfg_(logger *logger); // 这玩意的实现怎么不见了? 可能是准备实现...
    std::mutex logger_map_mutex_, flusher_mutex_;
    // std::recursive_mutex允许同一个线程多次获取该互斥锁，可以用来解决同一线程需要多次获取互斥量时死锁的问题
    std::recursive_mutex tp_mutex_;
    std::unordered_map<std::string, std::shared_ptr<logger>> loggers_;
    log_levels log_levels_;
    std::unique_ptr<formatter> formatter_;
    spdlog::level::level_enum global_log_level_ = level::info;
    level::level_enum flush_level_ = level::off;
    err_handler err_handler_;
    std::shared_ptr<thread_pool> tp_;
    std::unique_ptr<periodic_worker> periodic_flusher_;
    std::shared_ptr<logger> default_logger_;
    bool automatic_registration_ = true; // 是否在initialize_logger函数中自动注册logger
    size_t backtrace_n_messages_ = 0;
};

} // namespace details
} // namespace spdlog

#ifdef SPDLOG_HEADER_ONLY
#    include "registry-inl.h"
#endif
