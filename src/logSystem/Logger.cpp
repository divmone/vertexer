//
// Created by divmone on 10/17/2026.
//

#include "Logger.hpp"

#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

Logger::Logger(const std::string& appName, bool isDebug) {
    spdlog::init_thread_pool(8192, 1);
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console->set_level(isDebug ? spdlog::level::debug : spdlog::level::info);

    auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/" + appName + ".log", 10 * 1024 * 1024, 5);
    file->set_level(spdlog::level::trace);

    auto logger = std::make_shared<spdlog::async_logger>(
        appName, spdlog::sinks_init_list{console, file}, spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);

    logger->set_level(spdlog::level::trace);
    logger->set_pattern("%Y-%m-%d %H:%M:%S.%e [%^%l%$] [%t] %s:%# %v");
    logger->flush_on(spdlog::level::warn);
    spdlog::set_default_logger(logger);
    spdlog::flush_every(std::chrono::seconds(3));
}

Logger::~Logger() {
    spdlog::shutdown();
}