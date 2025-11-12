#ifndef ARCH_FEATURES_H
#define ARCH_FEATURES_H

#include <string>
#include <vector>
#include <cstdio>
#include <fstream>

class ArchFeatures {
public:
    static std::vector<std::string> get_pacman_logs() {
        std::vector<std::string> logs;
        FILE* pipe = popen("grep -E '(installed|upgraded|removed)' /var/log/pacman.log 2>/dev/null | tail -20", "r");
        if (pipe) {
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                logs.push_back(std::string(buffer));
            }
            pclose(pipe);
        }
        return logs;
    }
    
    static std::string get_system_info() {
        std::string info = "=== ARCH SYSTEM INFO ===\n";
        
        // Kernel version
        FILE* pipe = popen("uname -r 2>/dev/null", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                info += "Kernel: " + std::string(buffer);
            }
            pclose(pipe);
        }
        
        // Uptime
        pipe = popen("uptime -p 2>/dev/null", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                info += "Uptime: " + std::string(buffer);
            }
            pclose(pipe);
        }
        
        // Memory usage
        pipe = popen("free -h | grep Mem | awk '{print $3\"/\"$2}' 2>/dev/null", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                info += "Memory: " + std::string(buffer);
            }
            pclose(pipe);
        }
        
        return info;
    }
    
    static std::vector<std::string> get_failed_services() {
        std::vector<std::string> failed;
        FILE* pipe = popen("systemctl --failed --no-legend --no-pager 2>/dev/null | awk '{print $1}'", "r");
        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                std::string service(buffer);
                service.erase(service.find_last_not_of(" \n\r\t") + 1);
                if (!service.empty()) {
                    failed.push_back(service);
                }
            }
            pclose(pipe);
        }
        return failed;
    }
    
    static std::string get_boot_time() {
        FILE* pipe = popen("systemd-analyze time 2>/dev/null | head -1", "r");
        if (pipe) {
            char buffer[256];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                pclose(pipe);
                return std::string(buffer);
            }
            pclose(pipe);
        }
        return "Boot time: Unknown\n";
    }
    
    static bool is_aur_helper_available() {
        return (system("command -v yay >/dev/null 2>&1") == 0) || 
               (system("command -v paru >/dev/null 2>&1") == 0);
    }
};

#endif