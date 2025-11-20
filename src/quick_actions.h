#ifndef QUICK_ACTIONS_H
#define QUICK_ACTIONS_H

#include <string>
#include <vector>
#include <utility>

class QuickActions {
public:
    static std::string get_boot_errors() {
        return "journalctl -b -p err --no-pager -n 20";
    }
    
    static std::string get_kernel_messages() {
        return "journalctl -k --no-pager -n 20";
    }
    
    static std::string get_network_logs() {
        return "journalctl -u NetworkManager --no-pager -n 20";
    }
    
    static std::string get_auth_logs() {
        return "journalctl _COMM=sshd _COMM=sudo --no-pager -n 20";
    }
    
    static std::string get_gpu_logs() {
        return "journalctl _COMM=Xorg _COMM=nvidia --no-pager -n 20";
    }
    
    static std::vector<std::pair<std::string, std::string>> get_quick_filters() {
        return {
            {"🚨 Boot Errors", get_boot_errors()},
            {"🔧 Kernel Messages", get_kernel_messages()},
            {"🌐 Network Logs", get_network_logs()},
            {"🔐 Authentication", get_auth_logs()},
            {"🎮 GPU/Graphics", get_gpu_logs()}
        };
    }
};

#endif