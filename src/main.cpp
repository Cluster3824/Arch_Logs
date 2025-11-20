#include <iostream>
#include <string>
#include <vector>
#include <csignal>
#include <cstdlib>
#include "hardware_monitor.h"
#include "log_analyzer.h"
#include "arch_log_manager.h"
#include "system_compat.h"
#include "error_handler.h"

void print_usage() {
    std::cout << "ArchVault - System Log Analyzer\n";
    std::cout << "Usage: archlog [options]\n";
    std::cout << "  --summary        Show system summary\n";
    std::cout << "  -m LEVEL         Filter by log level (ERROR, WARNING, INFO)\n";
    std::cout << "  --tail=N         Show last N log entries\n";
    std::cout << "  --csv            Output in CSV format\n";
    std::cout << "  --no-filter      Show all logs without filtering\n";
    std::cout << "  --journal        Show systemd journal logs\n";
    std::cout << "  --service=NAME   Show logs for specific service\n";
    std::cout << "  --boot           Show boot logs\n";
    std::cout << "  --all-logs       Show all available Arch logs\n";
    std::cout << "  --help           Show this help message\n";
}

volatile sig_atomic_t interrupted = 0;

void signal_handler(int sig) {
    interrupted = 1;
    ErrorHandler::log_error("Received signal " + std::to_string(sig) + ", shutting down gracefully", ErrorLevel::INFO);
}

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    try {
        SystemCompat::validate_environment();
        ErrorHandler::log_error("ArchVault started on " + SystemCompat::get_system_info(), ErrorLevel::INFO);
        
        if (argc == 1) {
            print_usage();
            return 0;
        }
        
        std::string log_level = "";
        std::string service_name = "";
        int tail_count = 50;
        bool show_summary = false;
        bool csv_output = false;
        bool no_filter = false;
        bool show_journal = false;
        bool show_boot = false;
        bool show_all_logs = false;
        
        if (argc > 20) {
            throw ArchLogError("Too many arguments", ErrorLevel::ERROR);
        }
        
        for (int i = 1; i < argc && !interrupted; i++) {
            std::string arg = argv[i];
            if (arg.length() > 100) {
                throw ArchLogError("Argument too long", ErrorLevel::ERROR);
            }
            
            if (arg == "--help") {
                print_usage();
                return 0;
            } else if (arg == "--summary") {
                show_summary = true;
            } else if (arg == "-m" && i + 1 < argc) {
                log_level = argv[++i];
            } else if (arg.find("--tail=") == 0) {
                try {
                    tail_count = std::stoi(arg.substr(7));
                    tail_count = std::clamp(tail_count, 1, 10000);
                } catch (const std::exception& e) {
                    throw ArchLogError("Invalid tail count: " + arg, ErrorLevel::ERROR);
                }
            } else if (arg == "--csv") {
                csv_output = true;
            } else if (arg == "--no-filter") {
                no_filter = true;
            } else if (arg == "--journal") {
                show_journal = true;
            } else if (arg.find("--service=") == 0) {
                service_name = arg.substr(10);
            } else if (arg == "--boot") {
                show_boot = true;
            } else if (arg == "--all-logs") {
                show_all_logs = true;
            } else {
                throw ArchLogError("Unknown argument: " + arg, ErrorLevel::ERROR);
            }
        }
        
        if (show_summary && !interrupted) {
            std::cout << "=== System Hardware Summary ===\n";
            std::cout << "System: " << SystemCompat::get_system_info() << "\n";
            try {
                HardwareStats stats = HardwareMonitor::get_current_stats();
                std::cout << "CPU: " << stats.cpu_name << " (" << stats.cpu_usage << "% usage, " << stats.cpu_temp << "°C)\n";
                std::cout << "Memory: " << stats.memory_usage << "% used\n";
                std::cout << "Disk: " << stats.disk_usage << "% used\n";
                std::cout << "GPU: " << stats.gpu_name << " (" << stats.gpu_usage << "% usage, " << stats.gpu_temp << "°C)\n";
                std::cout << "System Load: " << stats.system_load << "\n";
                std::cout << "Network: RX " << stats.network_rx << " KB/s, TX " << stats.network_tx << " KB/s\n";
            } catch (const std::exception& e) {
                ErrorHandler::log_error("Failed to get hardware stats: " + std::string(e.what()), ErrorLevel::ERROR);
                return 1;
            }
        }
        
        // Analyze system logs
        std::cout << "\n=== System Log Analysis ===\n";
        try {
            std::vector<LogEntry> logs;
            
            if (show_all_logs) {
                logs = ArchLogManager::get_all_logs(tail_count);
                std::cout << "Showing all available Arch logs:\n";
            } else if (show_journal) {
                logs = ArchLogManager::get_journal_logs(tail_count);
                std::cout << "Showing systemd journal logs:\n";
            } else if (!service_name.empty()) {
                logs = ArchLogManager::get_service_logs(service_name, tail_count);
                std::cout << "Showing logs for service: " << service_name << "\n";
            } else if (show_boot) {
                logs = ArchLogManager::get_boot_logs();
                std::cout << "Showing boot logs:\n";
            } else {
                logs = LogAnalyzer::parse_logs("/var/log/syslog", tail_count);
                std::cout << "Showing syslog entries:\n";
            }
            
            if (!no_filter && !log_level.empty()) {
                logs = LogAnalyzer::filter_by_level(logs, log_level);
            }
            
            if (csv_output) {
                std::cout << "Timestamp,Level,Service,Message\n";
                for (const auto& entry : logs) {
                    std::cout << entry.timestamp << "," << entry.level << "," 
                             << entry.service << ",\"" << entry.message << "\"\n";
                }
            } else {
                for (const auto& entry : logs) {
                    std::cout << "[" << entry.timestamp << "] " << entry.level 
                             << " " << entry.service << ": " << entry.message << "\n";
                }
            }
            
            std::cout << "\nTotal entries: " << logs.size() << "\n";
            
        } catch (const ArchLogError& e) {
            ErrorHandler::log_error("Log analysis failed: " + std::string(e.what()), e.level());
            return 1;
        }
        
        if (!interrupted) {
            ErrorHandler::log_error("ArchVault completed successfully", ErrorLevel::INFO);
        }
        return interrupted ? 130 : 0;
        
    } catch (const ArchLogError& e) {
        ErrorHandler::log_error("ArchVault error: " + std::string(e.what()), e.level());
        return 1;
    } catch (const std::exception& e) {
        ErrorHandler::log_error("Unexpected error: " + std::string(e.what()), ErrorLevel::CRITICAL);
        return 1;
    } catch (...) {
        ErrorHandler::log_error("Unknown critical error occurred", ErrorLevel::CRITICAL);
        return 1;
    }
}