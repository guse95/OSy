#pragma once
#include <iostream>
#include <memory>
#include <string>
#include <vector>

class LoggerBuilder;

enum LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL,
};

class Logger {

    friend class LoggerBuilder;
    std::vector<std::ostream*> handlers;
    std::vector<std::unique_ptr<std::ostream>> own_handlers;

    Logger(): log_level(0) {
    }

    unsigned int log_level;

    static std::string levelToString(const unsigned int level)
    {
        switch (level) {
            case DEBUG:
                return " DEBUG: ";
            case INFO:
                return " INFO: ";
            case WARNING:
                return " WARNING: ";
            case ERROR:
                return " ERROR: ";
            case CRITICAL:
                return "CRITICAL: ";
            default:
                return " UNKNOWN: ";
        }
    }
public:
    void message(const unsigned int lvl, const std::string& str) {
        if (lvl >= log_level) {
            const time_t now = time(0);
            const tm* tp = localtime(&now);
            char timestamp[20];
            strftime(timestamp, sizeof(timestamp),"%Y:%m:%d %H:%M:%S", tp);
            for (auto&& iter : this->handlers) {
                (*iter) << timestamp << levelToString(lvl) << str << std::endl;
            }
        }
    }
    void critical(const std::string& msg) {
        message(CRITICAL, msg);
    }
    void error(const std::string& msg) {
        message(ERROR, msg);
    }
    void warning(const std::string& msg) {
        message(WARNING, msg);
    }
    void info(const std::string& msg) {
        message(INFO, msg);
    }
    void debug(const std::string& msg) {
        message(DEBUG, msg);
    }
};

class LoggerBuilder {

    Logger* logger = nullptr;

public:
    LoggerBuilder() { this->logger = new Logger(); }

    LoggerBuilder& set_level(unsigned int logging_level) {
        this->logger->log_level = logging_level;
        return *this;
    }

    LoggerBuilder& add_handler(std::ostream& stream) {
        this->logger->handlers.push_back(&stream);
        return *this;
    }

    LoggerBuilder& add_handler(std::unique_ptr<std::ostream> stream) {
        this->logger->handlers.push_back(stream.get());
        this->logger->own_handlers.push_back(std::move(stream));
        return *this;
    }

    [[nodiscard]] Logger* make_object() const {
        return this->logger;
    }
};