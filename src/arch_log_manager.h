#ifndef ARCH_LOG_MANAGER_H
#define ARCH_LOG_MANAGER_H

#include <string>
#include <vector>
#include <map>
#include <cstdio>
#include <memory>
#include <algorithm>
#include <unistd.h>
#include "error_handler.h"
#include "log_analyzer.h"

class ArchLogManager {
public:
    static std::vector<LogEntry> get_all_logs(int max_entries = 100) {
        std::vector<LogEntry> all_logs;
        
        // Get journalctl logs (systemd)
        auto journal_logs = get_journal_logs(max_entries / 2);
        all_logs.insert(all_logs.end(), journal_logs.begin(), journal_logs.end());
        
        // Get traditional log files
        auto file_logs = get_file_logs(max_entries / 2);
        all_logs.insert(all_logs.end(), file_logs.begin(), file_logs.end());
        
        return all_logs;
    }
    
    static std::vector<LogEntry> get_journal_logs(int max_entries = 50) {
        std::vector<LogEntry> logs;
        try {
            max_entries = std::clamp(max_entries, 1, 10000); // Prevent resource exhaustion
            std::string cmd = "timeout 30 journalctl -n " + std::to_string(max_entries) + " --no-pager -o short --no-hostname 2>/dev/null";
            
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
            if (!pipe) {
                ErrorHandler::handle_system_error("journalctl execution");
                return logs;
            }
            
            char buffer[2048];
            int line_count = 0;
            while (fgets(buffer, sizeof(buffer), pipe.get()) && line_count < max_entries) {
                buffer[2047] = '\0'; // Ensure null termination
                std::string line(buffer);
                LogEntry entry = parse_journal_line(line);
                if (!entry.timestamp.empty()) {
                    logs.push_back(std::move(entry));
                    line_count++;
                }
            }
        } catch (const std::exception& e) {
            ErrorHandler::log_error("Journal log access failed: " + std::string(e.what()), ErrorLevel::WARNING);
        }
        return logs;
    }
    
    static std::vector<LogEntry> get_file_logs(int max_entries = 50) {
        std::vector<LogEntry> logs;
        std::vector<std::string> log_files = {
            "/var/log/syslog", "/var/log/messages", "/var/log/kern.log",
            "/var/log/auth.log", "/var/log/daemon.log", "/var/log/user.log"
        };
        
        for (const auto& file : log_files) {
            try {
                auto file_logs = LogAnalyzer::parse_logs(file, max_entries / log_files.size());
                logs.insert(logs.end(), file_logs.begin(), file_logs.end());
            } catch (const std::exception& e) {
                // Continue with other files if one fails
                continue;
            }
        }
        return logs;
    }
    
    static std::vector<LogEntry> get_service_logs(const std::string& service, int max_entries = 50) {
        std::vector<LogEntry> logs;
        try {
            // Input validation for service name
            if (service.empty() || service.length() > 100 || 
                service.find_first_of(";|&`$(){}[]<>*?\\") != std::string::npos) {
                ErrorHandler::log_error("Invalid service name: " + service, ErrorLevel::WARNING);
                return logs;
            }
            
            max_entries = std::clamp(max_entries, 1, 5000);
            std::string cmd = "timeout 20 journalctl -u '" + service + "' -n " + 
                            std::to_string(max_entries) + " --no-pager -o short --no-hostname 2>/dev/null";
            
            std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
            if (!pipe) {
                ErrorHandler::handle_system_error("service log access for " + service);
                return logs;
            }
            
            char buffer[2048];
            int line_count = 0;
            while (fgets(buffer, sizeof(buffer), pipe.get()) && line_count < max_entries) {
                buffer[2047] = '\0';
                std::string line(buffer);
                LogEntry entry = parse_journal_line(line);
                if (!entry.timestamp.empty()) {
                    logs.push_back(std::move(entry));
                    line_count++;
                }
            }
        } catch (const std::exception& e) {
            ErrorHandler::log_error("Service log access failed for " + service + ": " + std::string(e.what()), ErrorLevel::WARNING);
        }
        return logs;
    }
    
    static std::vector<LogEntry> get_boot_logs() {
        std::vector<LogEntry> logs;
        try {
            std::unique_ptr<FILE, decltype(&pclose)> pipe(
                popen("timeout 60 journalctl -b --no-pager -o short --no-hostname -n 1000 2>/dev/null", "r"), pclose);
            if (!pipe) {
                ErrorHandler::handle_system_error("boot log access");
                return logs;
            }
            
            char buffer[2048];
            int line_count = 0;
            while (fgets(buffer, sizeof(buffer), pipe.get()) && line_count < 1000) {
                buffer[2047] = '\0';
                std::string line(buffer);
                LogEntry entry = parse_journal_line(line);
                if (!entry.timestamp.empty()) {
                    logs.push_back(std::move(entry));
                    line_count++;
                }
            }
        } catch (const std::exception& e) {
            ErrorHandler::log_error("Boot log access failed: " + std::string(e.what()), ErrorLevel::WARNING);
        }
        return logs;
    }

private:
    static LogEntry parse_journal_line(const std::string& line) {
        LogEntry entry;
        if (line.length() < 20) return entry;
        
        // Parse journalctl format: "Jan 01 12:00:00 hostname service[pid]: message"
        size_t first_space = line.find(' ');
        size_t second_space = line.find(' ', first_space + 1);
        size_t third_space = line.find(' ', second_space + 1);
        size_t fourth_space = line.find(' ', third_space + 1);
        
        if (fourth_space != std::string::npos) {
            entry.timestamp = line.substr(0, second_space + 9); // Include time
            
            size_t colon_pos = line.find(':', fourth_space);
            if (colon_pos != std::string::npos) {
                std::string service_part = line.substr(fourth_space + 1, colon_pos - fourth_space - 1);
                size_t bracket_pos = service_part.find('[');
                entry.service = (bracket_pos != std::string::npos) ? 
                    service_part.substr(0, bracket_pos) : service_part;
                
                entry.message = line.substr(colon_pos + 2);
                
                // Determine log level
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
        }
        return entry;
    }
};

#endif