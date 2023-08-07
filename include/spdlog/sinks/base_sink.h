// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once
//
// base sink templated over a mutex (either dummy or real)
// concrete implementation should override the sink_it_() and flush_()  methods.
// locking is taken care of in this class - no locking needed by the
// implementers..
//

#include <spdlog/common.h>
#include <spdlog/details/log_msg.h>
#include <spdlog/sinks/sink.h>

namespace spdlog {
namespace sinks {
template<typename Mutex>
class SPDLOG_API base_sink : public sink
{
public:
    base_sink();
    // explicit禁止构造函数被用于隐式转换
    explicit base_sink(std::unique_ptr<spdlog::formatter> formatter);
    ~base_sink() override = default;

    base_sink(const base_sink &) = delete;
    base_sink(base_sink &&) = delete;

    base_sink &operator=(const base_sink &) = delete;
    base_sink &operator=(base_sink &&) = delete;

    // 下面这几个是继承来的纯虚函数。final阻止了这些虚函数被子类重载
    // 继承来的4个虚函数加锁地调用protected里面的4个函数
    void log(const details::log_msg &msg) final;
    void flush() final;
    void set_pattern(const std::string &pattern) final;
    void set_formatter(std::unique_ptr<spdlog::formatter> sink_formatter) final;

protected:
    // sink formatter
    std::unique_ptr<spdlog::formatter> formatter_;
    Mutex mutex_;

    virtual void sink_it_(const details::log_msg &msg) = 0;
    virtual void flush_() = 0;
    virtual void set_pattern_(const std::string &pattern);
    virtual void set_formatter_(std::unique_ptr<spdlog::formatter> sink_formatter);
};
} // namespace sinks
} // namespace spdlog

#ifdef SPDLOG_HEADER_ONLY
#    include "base_sink-inl.h"
#endif
