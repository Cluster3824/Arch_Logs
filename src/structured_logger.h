#ifndef STRUCTURED_LOGGER_H
#define STRUCTURED_LOGGER_H

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <iostream>
#include <cstdlib>
#include <ctime>

enum class LogLevel {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    FATAL = 5
};

enum class LogCategory {
    SYSTEM,
    SECURITY,
    NETWORK,
    HARDWARE,
    APPLICATION,
    USER_ACTION,
    PERFORMANCE
};

struct StructuredLogEntry {
    std::string timestamp;
    std::string log_name;
    std::string directory;
    LogLevel level;
    LogCategory category;
    std::string message;
    std::string source_file;
    int line_number;
    std::string user;
    std::string session_id;
    std::map<std::string, std::string> metadata;
    
    std::string to_json() const {
        std::stringstream json;
        json << "{"
             << "\"timestamp\":\"" << timestamp << "\","
             << "\"log_name\":\"" << log_name << "\","
             << "\"directory\":\"" << directory << "\","
             << "\"level\":\"" << level_to_string(level) << "\","
             << "\"category\":\"" << category_to_string(category) << "\","
             << "\"message\":\"" << escape_json(message) << "\","
             << "\"source\":\"" << source_file << ":" << line_number << "\","
             << "\"user\":\"" << user << "\","
             << "\"session\":\"" << session_id << "\"";
        
        if (!metadata.empty()) {
            json << ",\"metadata\":{";
            bool first = true;
            for (const auto& [key, value] : metadata) {
                if (!first) json << ",";
                json << "\"" << key << "\":\"" << escape_json(value) << "\"";
                first = false;
            }
            json << "}";
        }
        
        json << "}";
        return json.str();
    }
    
    std::string to_formatted_string() const {
        std::stringstream ss;
        ss << "[" << timestamp << "] "
           << "[" << level_to_string(level) << "] "
           << "[" << category_to_string(category) << "] "
           << log_name << " (" << directory << ") "
           << "| " << message;
        
        if (!metadata.empty()) {
            ss << " {";
            bool first = true;
            for (const auto& [key, value] : metadata) {
                if (!first) ss << ", ";
                ss << key << "=" << value;
                first = false;
            }
            ss << "}";
        }
        
        return ss.str();
    }

private:
    std::string level_to_string(LogLevel level) const {
        switch (level) {
            case LogLevel::TRACE: return "TRACE";
            case LogLevel::DEBUG: return "DEBUG";
            case LogLevel::INFO: return "INFO";
            case LogLevel::WARN: return "WARN";
            case LogLevel::ERROR: return "ERROR";
            case LogLevel::FATAL: return "FATAL";
            default: return "UNKNOWN";
        }
    }
    
    std::string category_to_string(LogCategory cat) const {
        switch (cat) {
            case LogCategory::SYSTEM: return "SYSTEM";
            case LogCategory::SECURITY: return "SECURITY";
            case LogCategory::NETWORK: return "NETWORK";
            case LogCategory::HARDWARE: return "HARDWARE";
            case LogCategory::APPLICATION: return "APPLICATION";
            case LogCategory::USER_ACTION: return "USER_ACTION";
            case LogCategory::PERFORMANCE: return "PERFORMANCE";
            default: return "UNKNOWN";
        }
    }
    
    std::string escape_json(const std::string& str) const {
        std::string escaped;
        for (char c : str) {
            switch (c) {
                case '"': escaped += "\\\""; break;
                case '\\': escaped += "\\\\"; break;
                case '\n': escaped += "\\n"; break;
                case '\r': escaped += "\\r"; break;
                case '\t': escaped += "\\t"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    }
};

class StructuredLogger {
private:
    static std::string session_id;
    static std::string current_user;
    static LogLevel min_level;
    static bool json_output;
    static std::string log_directory;

public:
    static void initialize(const std::string& user = "", const std::string& log_dir = "/tmp/archlog") {
        current_user = user.empty() ? get_current_user() : user;
        session_id = generate_session_id();
        log_directory = log_dir;
        min_level = LogLevel::INFO;
        json_output = false;
    }
    
    static void set_level(LogLevel level) { min_level = level; }
    static void set_json_output(bool enable) { json_output = enable; }
    
    static void log(LogLevel level, LogCategory category, const std::string& log_name,
                   const std::string& directory, const std::string& message,
                   const std::string& file = "", int line = 0,
                   const std::map<std::string, std::string>& metadata = {}) {
        
        if (level < min_level) return;
        
        StructuredLogEntry entry;
        entry.timestamp = get_current_timestamp();
        entry.log_name = log_name;
        entry.directory = directory;
        entry.level = level;
        entry.category = category;
        entry.message = message;
        entry.source_file = file;
        entry.line_number = line;
        entry.user = current_user;
        entry.session_id = session_id;
        entry.metadata = metadata;
        
        output_log(entry);
    }
    
    // Convenience methods
    static void trace(const std::string& log_name, const std::string& dir, const std::string& msg) {
        log(LogLevel::TRACE, LogCategory::APPLICATION, log_name, dir, msg);
    }
    
    static void debug(const std::string& log_name, const std::string& dir, const std::string& msg) {
        log(LogLevel::DEBUG, LogCategory::APPLICATION, log_name, dir, msg);
    }
    
    static void info(const std::string& log_name, const std::string& dir, const std::string& msg) {
        log(LogLevel::INFO, LogCategory::APPLICATION, log_name, dir, msg);
    }
    
    static void warn(const std::string& log_name, const std::string& dir, const std::string& msg) {
        log(LogLevel::WARN, LogCategory::APPLICATION, log_name, dir, msg);
    }
    
    static void error(const std::string& log_name, const std::string& dir, const std::string& msg) {
        log(LogLevel::ERROR, LogCategory::APPLICATION, log_name, dir, msg);
    }
    
    static void security(const std::string& log_name, const std::string& dir, const std::string& msg) {
        log(LogLevel::WARN, LogCategory::SECURITY, log_name, dir, msg);
    }
    
    static void system(const std::string& log_name, const std::string& dir, const std::string& msg) {
        log(LogLevel::INFO, LogCategory::SYSTEM, log_name, dir, msg);
    }
    
    static void network(const std::string& log_name, const std::string& dir, const std::string& msg) {
        log(LogLevel::INFO, LogCategory::NETWORK, log_name, dir, msg);
    }
    
    static void hardware(const std::string& log_name, const std::string& dir, const std::string& msg) {
        log(LogLevel::INFO, LogCategory::HARDWARE, log_name, dir, msg);
    }
    
    static void performance(const std::string& log_name, const std::string& dir, const std::string& msg,
                          const std::map<std::string, std::string>& metrics = {}) {
        log(LogLevel::INFO, LogCategory::PERFORMANCE, log_name, dir, msg, "", 0, metrics);
    }
    
    static void user_action(const std::string& action, const std::string& details = "") {
        std::map<std::string, std::string> metadata;
        if (!details.empty()) metadata["details"] = details;
        log(LogLevel::INFO, LogCategory::USER_ACTION, "user_interface", "/gui", action, "", 0, metadata);
    }

private:
    static std::string get_current_timestamp() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    static std::string generate_session_id() {
        auto now = std::chrono::high_resolution_clock::now();
        auto timestamp = now.time_since_epoch().count();
        return "sess_" + std::to_string(timestamp);
    }
    
    static std::string get_current_user() {
        const char* user = getenv("USER");
        return user ? std::string(user) : "unknown";
    }
    
    static void output_log(const StructuredLogEntry& entry) {
        std::string output = json_output ? entry.to_json() : entry.to_formatted_string();
        
        // Output to console
        std::cout << output << std::endl;
        
        // Also save to file
        std::string filename = log_directory + "/archlog_" + 
                              std::to_string(std::chrono::system_clock::to_time_t(
                                  std::chrono::system_clock::now())) + ".log";
        
        std::ofstream logfile(filename, std::ios::app);
        if (logfile.is_open()) {
            logfile << output << std::endl;
            logfile.close();
        }
    }
};

// Static member definitions
std::string StructuredLogger::session_id;
std::string StructuredLogger::current_user;
LogLevel StructuredLogger::min_level = LogLevel::INFO;
bool StructuredLogger::json_output = false;
std::string StructuredLogger::log_directory = "/tmp/archlog";

#endif