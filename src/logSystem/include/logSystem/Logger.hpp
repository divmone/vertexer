//
// Created by divmone on 10/17/2026.
//

#pragma once

#include <spdlog/spdlog.h>

class Logger {
public:
    Logger(const std::string& appName, bool isDebug);
    ~Logger();

    Logger(const Logger&)            = delete;
    Logger& operator=(const Logger&) = delete;
};

#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)