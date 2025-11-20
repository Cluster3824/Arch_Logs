#ifndef HARDWARE_MONITOR_H
#define HARDWARE_MONITOR_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <utility>
#include <iostream>
#include <memory>
#include <algorithm>
#include <limits>
#include <unistd.h>
#include <sys/stat.h>
#include "error_handler.h"

struct HardwareStats {
    double cpu_usage = 0.0;
    double memory_usage = 0.0;
    double disk_usage = 0.0;
    double gpu_usage = 0.0;
    std::string system_load = "0.0";
    int active_connections = 0;
    int cpu_temp = 0;
    int gpu_temp = 0;
    double network_rx = 0.0;
    double network_tx = 0.0;
    std::string gpu_name = "Unknown";
    std::string cpu_name = "Unknown";
};

class HardwareMonitor {
public:
    static HardwareStats get_current_stats() {
        HardwareStats stats;
        stats.cpu_usage = get_cpu_usage();
        stats.memory_usage = get_memory_usage();
        stats.disk_usage = get_disk_usage();
        stats.gpu_usage = get_gpu_usage();
        stats.cpu_temp = get_cpu_temperature();
        stats.gpu_temp = get_gpu_temperature();
        stats.system_load = get_system_load();
        stats.active_connections = get_active_connections();
        auto network = get_network_stats();
        stats.network_rx = network.first;
        stats.network_tx = network.second;
        stats.gpu_name = get_gpu_name();
        stats.cpu_name = get_cpu_name();
        return stats;
    }

private:
    static double get_gpu_usage() {
        try {
            // Try reading AMD GPU usage directly (more secure)
            std::ifstream amd_gpu("/sys/class/drm/card0/device/gpu_busy_percent");
            if (amd_gpu.is_open()) {
                std::string usage_str;
                if (std::getline(amd_gpu, usage_str) && !usage_str.empty()) {
                    return std::clamp(std::stod(usage_str), 0.0, 100.0);
                }
            }
            
            // Fallback to nvidia-smi with input validation
            std::unique_ptr<FILE, decltype(&pclose)> pipe(
                popen("timeout 5 nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null", "r"), pclose);
            if (pipe) {
                char buffer[32];
                if (fgets(buffer, sizeof(buffer), pipe.get())) {
                    buffer[31] = '\0'; // Ensure null termination
                    double usage = std::stod(std::string(buffer));
                    return std::clamp(usage, 0.0, 100.0);
                }
            }
        } catch (const std::exception& e) {
            ErrorHandler::log_error("GPU usage detection failed: " + std::string(e.what()), ErrorLevel::WARNING);
        }
        return 0.0;
    }
    
    static std::string get_system_load() {
        try {
            std::ifstream loadavg("/proc/loadavg");
            if (!loadavg.is_open()) {
                ErrorHandler::handle_file_error("/proc/loadavg", "read");
                return "0.0";
            }
            std::string load;
            loadavg >> load;
            return load;
        } catch (const std::exception& e) {
            ErrorHandler::log_error("System load detection failed: " + std::string(e.what()), ErrorLevel::WARNING);
            return "0.0";
        }
    }
    
    static int get_active_connections() {
        try {
            // More secure: read /proc/net/tcp directly
            std::ifstream tcp_file("/proc/net/tcp");
            if (!tcp_file.is_open()) return 0;
            
            int count = 0;
            std::string line;
            std::getline(tcp_file, line); // Skip header
            
            while (std::getline(tcp_file, line) && count < 10000) { // Prevent infinite loop
                if (line.find(" 0A ") != std::string::npos) { // LISTEN state
                    count++;
                }
            }
            return count;
        } catch (const std::exception& e) {
            ErrorHandler::log_error("Connection count failed: " + std::string(e.what()), ErrorLevel::WARNING);
            return 0;
        }
    }
    static double get_cpu_usage() {
        try {
            // More efficient /proc/stat parsing
            std::ifstream stat_file("/proc/stat");
            if (!stat_file.is_open()) {
                ErrorHandler::handle_file_error("/proc/stat", "read");
                return 0.0;
            }
            
            std::string line;
            if (std::getline(stat_file, line) && line.substr(0, 3) == "cpu") {
                std::istringstream iss(line);
                std::string cpu;
                long user, nice, system, idle, iowait, irq, softirq;
                iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
                
                long total = user + nice + system + idle + iowait + irq + softirq;
                long active = total - idle - iowait;
                
                static long prev_total = 0, prev_active = 0;
                if (prev_total > 0) {
                    double usage = 100.0 * (active - prev_active) / (total - prev_total);
                    prev_total = total;
                    prev_active = active;
                    return std::clamp(usage, 0.0, 100.0);
                }
                prev_total = total;
                prev_active = active;
            }
        } catch (const std::exception& e) {
            ErrorHandler::log_error("CPU usage detection failed: " + std::string(e.what()), ErrorLevel::WARNING);
        }
        return 0.0;
    }

    static double get_memory_usage() {
        try {
            std::ifstream meminfo("/proc/meminfo");
            if (!meminfo.is_open()) {
                ErrorHandler::handle_file_error("/proc/meminfo", "read");
                return 0.0;
            }
            
            long total = 0, available = 0;
            std::string line;
            while (std::getline(meminfo, line)) {
                if (line.find("MemTotal:") == 0) {
                    std::istringstream iss(line);
                    std::string key, kb;
                    iss >> key >> total >> kb;
                } else if (line.find("MemAvailable:") == 0) {
                    std::istringstream iss(line);
                    std::string key, kb;
                    iss >> key >> available >> kb;
                    break;
                }
            }
            return total > 0 ? ((total - available) * 100.0 / total) : 0.0;
        } catch (const std::exception& e) {
            ErrorHandler::log_error("Memory usage detection failed: " + std::string(e.what()), ErrorLevel::WARNING);
            return 0.0;
        }
    }

    static double get_disk_usage() {
        try {
            // Use statvfs for better performance and security
            struct statvfs stat;
            if (statvfs("/", &stat) == 0) {
                double total = static_cast<double>(stat.f_blocks) * stat.f_frsize;
                double free = static_cast<double>(stat.f_bavail) * stat.f_frsize;
                double used = total - free;
                return std::clamp((used / total) * 100.0, 0.0, 100.0);
            }
        } catch (const std::exception& e) {
            ErrorHandler::log_error("Disk usage detection failed: " + std::string(e.what()), ErrorLevel::WARNING);
        }
        return 0.0;
    }

    static int get_cpu_temperature() {
        std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
        if (!temp_file.is_open()) return 0;
        int temp_millidegree;
        temp_file >> temp_millidegree;
        return temp_millidegree / 1000;
    }

    static int get_gpu_temperature() {
        try {
            // Try AMD GPU temperature files directly
            for (int i = 0; i < 10; i++) {
                std::string path = "/sys/class/drm/card" + std::to_string(i) + "/device/hwmon";
                std::ifstream hwmon_dir(path);
                if (hwmon_dir.good()) {
                    for (int j = 0; j < 10; j++) {
                        std::string temp_path = path + "/hwmon" + std::to_string(j) + "/temp1_input";
                        std::ifstream temp_file(temp_path);
                        if (temp_file.is_open()) {
                            int temp_millidegree;
                            if (temp_file >> temp_millidegree) {
                                return std::clamp(temp_millidegree / 1000, 0, 150);
                            }
                        }
                    }
                }
            }
            
            // Fallback to nvidia-smi with timeout
            std::unique_ptr<FILE, decltype(&pclose)> pipe(
                popen("timeout 3 nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits 2>/dev/null", "r"), pclose);
            if (pipe) {
                char buffer[16];
                if (fgets(buffer, sizeof(buffer), pipe.get())) {
                    buffer[15] = '\0';
                    int temp = std::stoi(std::string(buffer));
                    return std::clamp(temp, 0, 150);
                }
            }
        } catch (const std::exception& e) {
            ErrorHandler::log_error("GPU temperature detection failed: " + std::string(e.what()), ErrorLevel::WARNING);
        }
        return 0;
    }

    static std::pair<double, double> get_network_stats() {
        FILE* pipe = popen("cat /proc/net/dev | grep -E '(eth|wlan|enp|wlp)' | head -1 | awk '{print $2, $10}'", "r");
        if (!pipe) return {0.0, 0.0};
        char buffer[128];
        double rx = 0.0, tx = 0.0;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            std::string data(buffer);
            std::istringstream iss(data);
            iss >> rx >> tx;
            rx /= 1024.0;
            tx /= 1024.0;
        }
        pclose(pipe);
        return {rx, tx};
    }

    static std::string get_gpu_name() {
        FILE* pipe = popen("lspci | grep -i vga | cut -d':' -f3 | head -1", "r");
        if (!pipe) return "Unknown";
        char buffer[256];
        std::string name = "Unknown";
        if (fgets(buffer, sizeof(buffer), pipe)) {
            name = std::string(buffer);
            name.erase(name.find_last_not_of(" \n\r\t") + 1);
        }
        pclose(pipe);
        return name;
    }

    static std::string get_cpu_name() {
        std::ifstream cpuinfo("/proc/cpuinfo");
        if (!cpuinfo.is_open()) return "Unknown";
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") == 0) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string name = line.substr(pos + 2);
                    return name;
                }
            }
        }
        return "Unknown";
    }
};

#endif