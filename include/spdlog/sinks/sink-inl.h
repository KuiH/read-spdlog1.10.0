// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)

#pragma once

#ifndef SPDLOG_HEADER_ONLY
#    include <spdlog/sinks/sink.h>
#endif

#include <spdlog/common.h>

SPDLOG_INLINE bool spdlog::sinks::sink::should_log(spdlog::level::level_enum msg_level) const
{
    // std::memory_order_relaxed: 仅仅保证load()和store()是原子操作，除此之外，不提供任何跨线程的同步
    return msg_level >= level_.load(std::memory_order_relaxed);
}

SPDLOG_INLINE void spdlog::sinks::sink::set_level(level::level_enum log_level)
{
    level_.store(log_level, std::memory_order_relaxed);
}

SPDLOG_INLINE spdlog::level::level_enum spdlog::sinks::sink::level() const
{
    return static_cast<spdlog::level::level_enum>(level_.load(std::memory_order_relaxed));
}
