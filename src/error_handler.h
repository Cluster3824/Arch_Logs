#ifndef ERROR_HANDLER_H
#define ERROR_HANDLER_H

#include <string>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <chrono>
#include <ctime>

enum class ErrorLevel {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class ArchLogError : public std::runtime_error {
public:
    ArchLogError(const std::string& msg, ErrorLevel level = ErrorLevel::ERROR) 
        : std::runtime_error(msg), level_(level) {}
    
    ErrorLevel level() const { return level_; }

private:
    ErrorLevel level_;
};

class ErrorHandler {
public:
    static void log_error(const std::string& message, ErrorLevel level = ErrorLevel::ERROR) {
        std::string level_str = level_to_string(level);
        std::string timestamp = get_timestamp();
        
        std::cerr << "[" << timestamp << "] " << level_str << ": " << message << std::endl;
        
        // Log to file if possible
        std::ofstream log_file("/tmp/archlog_errors.log", std::ios::app);
        if (log_file.is_open()) {
            log_file << "[" << timestamp << "] " << level_str << ": " << message << std::endl;
        }
    }
    
    static void handle_system_error(const std::string& operation, int error_code = 0) {
        std::string msg = "System operation failed: " + operation;
        if (error_code != 0) {
            msg += " (code: " + std::to_string(error_code) + ")";
        }
        log_error(msg, ErrorLevel::ERROR);
    }
    
    static void handle_file_error(const std::string& filepath, const std::string& operation) {
        std::string msg = "File operation failed: " + operation + " on " + filepath;
        log_error(msg, ErrorLevel::WARNING);
    }

private:
    static std::string level_to_string(ErrorLevel level) {
        switch (level) {
            case ErrorLevel::INFO: return "INFO";
            case ErrorLevel::WARNING: return "WARN";
            case ErrorLevel::ERROR: return "ERROR";
            case ErrorLevel::CRITICAL: return "CRITICAL";
            default: return "UNKNOWN";
        }
    }
    
    static std::string get_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        char buffer[20];
        std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&time_t));
        return std::string(buffer);
    }
};

#endif