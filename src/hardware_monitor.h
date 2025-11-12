#ifndef HARDWARE_MONITOR_H
#define HARDWARE_MONITOR_H

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <cstdio>

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
        // Try nvidia-smi first
        FILE* pipe = popen("nvidia-smi --query-gpu=utilization.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
        if (pipe) {
            char buffer[64];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                pclose(pipe);
                try {
                    return std::stod(std::string(buffer));
                } catch (...) {}
            }
            pclose(pipe);
        }
        
        // Try AMD GPU monitoring
        pipe = popen("cat /sys/class/drm/card0/device/gpu_busy_percent 2>/dev/null", "r");
        if (pipe) {
            char buffer[64];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                pclose(pipe);
                try {
                    return std::stod(std::string(buffer));
                } catch (...) {}
            }
            pclose(pipe);
        }
        
        return 0.0;
    }
    
    static std::string get_system_load() {
        std::ifstream loadavg("/proc/loadavg");
        if (!loadavg.is_open()) return "0.0";
        std::string load;
        loadavg >> load;
        return load;
    }
    
    static int get_active_connections() {
        FILE* pipe = popen("ss -tuln | grep LISTEN | wc -l 2>/dev/null", "r");
        if (!pipe) return 0;
        char buffer[32];
        int count = 0;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            count = std::stoi(std::string(buffer));
        }
        pclose(pipe);
        return count;
    }
    static double get_cpu_usage() {
        FILE* pipe = popen("top -bn1 | grep 'Cpu(s)' | awk '{print $2}' | cut -d'%' -f1", "r");
        if (!pipe) return 0.0;
        char buffer[64];
        double usage = 0.0;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            usage = std::stod(std::string(buffer));
        }
        pclose(pipe);
        return usage;
    }

    static double get_memory_usage() {
        std::ifstream meminfo("/proc/meminfo");
        if (!meminfo.is_open()) return 0.0;
        
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
    }

    static double get_disk_usage() {
        FILE* pipe = popen("df / | tail -1 | awk '{print $5}' | cut -d'%' -f1", "r");
        if (!pipe) return 0.0;
        char buffer[64];
        double usage = 0.0;
        if (fgets(buffer, sizeof(buffer), pipe)) {
            usage = std::stod(std::string(buffer));
        }
        pclose(pipe);
        return usage;
    }

    static int get_cpu_temperature() {
        std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
        if (!temp_file.is_open()) return 0;
        int temp_millidegree;
        temp_file >> temp_millidegree;
        return temp_millidegree / 1000;
    }

    static int get_gpu_temperature() {
        // Try nvidia-smi first
        FILE* pipe = popen("nvidia-smi --query-gpu=temperature.gpu --format=csv,noheader,nounits 2>/dev/null", "r");
        if (pipe) {
            char buffer[64];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                pclose(pipe);
                try {
                    return std::stoi(std::string(buffer));
                } catch (...) {}
            }
            pclose(pipe);
        }
        
        // Try AMD GPU temperature
        pipe = popen("cat /sys/class/drm/card0/device/hwmon/hwmon*/temp1_input 2>/dev/null | head -1", "r");
        if (pipe) {
            char buffer[64];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                pclose(pipe);
                try {
                    return std::stoi(std::string(buffer)) / 1000;
                } catch (...) {}
            }
            pclose(pipe);
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