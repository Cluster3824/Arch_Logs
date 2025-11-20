#ifndef LOG_ANALYZER_H
#define LOG_ANALYZER_H

#include <string>
#include <vector>
#include <fstream>
#include <regex>
#include "error_handler.h"

struct LogEntry {
    std::string timestamp;
    std::string level;
    std::string service;
    std::string message;
};

class LogAnalyzer {
public:
    static std::vector<LogEntry> parse_logs(const std::string& log_path, int max_lines = 100) {
        std::vector<LogEntry> entries;
        
        try {
            std::ifstream log_file(log_path);
            if (!log_file.is_open()) {
                throw ArchLogError("Cannot open log file: " + log_path, ErrorLevel::ERROR);
            }
            
            std::string line;
            int line_count = 0;
            
            while (std::getline(log_file, line) && line_count < max_lines) {
                try {
                    LogEntry entry = parse_log_line(line);
                    if (!entry.timestamp.empty()) {
                        entries.push_back(entry);
                        line_count++;
                    }
                } catch (const std::exception& e) {
                    ErrorHandler::log_error("Failed to parse log line: " + std::string(e.what()), ErrorLevel::WARNING);
                }
            }
            
            if (entries.empty()) {
                ErrorHandler::log_error("No valid log entries found in: " + log_path, ErrorLevel::WARNING);
            }
            
        } catch (const ArchLogError& e) {
            ErrorHandler::log_error(e.what(), e.level());
            throw;
        } catch (const std::exception& e) {
            ErrorHandler::log_error("Unexpected error parsing logs: " + std::string(e.what()), ErrorLevel::ERROR);
            throw ArchLogError("Log parsing failed", ErrorLevel::ERROR);
        }
        
        return entries;
    }
    
    static std::vector<LogEntry> filter_by_level(const std::vector<LogEntry>& entries, const std::string& level) {
        std::vector<LogEntry> filtered;
        
        try {
            for (const auto& entry : entries) {
                if (entry.level == level) {
                    filtered.push_back(entry);
                }
            }
        } catch (const std::exception& e) {
            ErrorHandler::log_error("Error filtering logs by level: " + std::string(e.what()), ErrorLevel::WARNING);
        }
        
        return filtered;
    }

private:
    static LogEntry parse_log_line(const std::string& line) {
        LogEntry entry;
        
        // Basic systemd journal format parsing
        std::regex log_regex(R"((\w+\s+\d+\s+\d+:\d+:\d+)\s+\w+\s+(\w+)(?:\[\d+\])?\s*:\s*(.+))");
        std::smatch matches;
        
        if (std::regex_search(line, matches, log_regex)) {
            entry.timestamp = matches[1].str();
            entry.service = matches[2].str();
            entry.message = matches[3].str();
            
            // Determine log level from message content
            std::string msg_lower = entry.message;
            std::transform(msg_lower.begin(), msg_lower.end(), msg_lower.begin(), ::tolower);
            
            if (msg_lower.find("error") != std::string::npos || msg_lower.find("failed") != std::string::npos) {
                entry.level = "ERROR";
            } else if (msg_lower.find("warning") != std::string::npos || msg_lower.find("warn") != std::string::npos) {
                entry.level = "WARNING";
            } else {
                entry.level = "INFO";
            }
        }
        
        return entry;
    }
};

#endif