#ifndef SYSTEM_COMPAT_H
#define SYSTEM_COMPAT_H

#include <string>
#include <fstream>
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include "error_handler.h"

class SystemCompat {
public:
    static bool is_arch_linux() {
        std::ifstream os_release("/etc/os-release");
        if (!os_release.is_open()) return false;
        
        std::string line;
        while (std::getline(os_release, line)) {
            if (line.find("ID=arch") != std::string::npos || 
                line.find("ID_LIKE=arch") != std::string::npos) {
                return true;
            }
        }
        return false;
    }
    
    static bool has_systemd() {
        return access("/run/systemd/system", F_OK) == 0;
    }
    
    static bool has_journalctl() {
        return access("/usr/bin/journalctl", X_OK) == 0 || 
               access("/bin/journalctl", X_OK) == 0;
    }
    
    static std::string get_system_info() {
        struct utsname sys_info;
        if (uname(&sys_info) != 0) {
            return "Unknown system";
        }
        
        return std::string(sys_info.sysname) + " " + 
               std::string(sys_info.release) + " " + 
               std::string(sys_info.machine);
    }
    
    static bool check_permissions() {
        // Check if we can read system files
        return access("/proc/stat", R_OK) == 0 && 
               access("/proc/meminfo", R_OK) == 0;
    }
    
    static void validate_environment() {
        if (!check_permissions()) {
            throw ArchLogError("Insufficient permissions to read system files", ErrorLevel::CRITICAL);
        }
        
        if (!has_systemd()) {
            ErrorHandler::log_error("systemd not detected - some features may be limited", ErrorLevel::WARNING);
        }
        
        if (!has_journalctl()) {
            ErrorHandler::log_error("journalctl not found - journal logs unavailable", ErrorLevel::WARNING);
        }
        
        if (!is_arch_linux()) {
            ErrorHandler::log_error("Non-Arch system detected - some paths may differ", ErrorLevel::INFO);
        }
    }
};

#endif