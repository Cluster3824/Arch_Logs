#ifndef ENHANCED_SECURITY_H
#define ENHANCED_SECURITY_H

#include <string>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include <ctime>

class EnhancedSecurity {
public:
    static bool is_running_as_root() {
        return getuid() == 0;
    }
    
    static std::string get_current_user() {
        struct passwd *pw = getpwuid(getuid());
        return pw ? std::string(pw->pw_name) : "unknown";
    }
    
    static bool validate_file_permissions(const std::string& path) {
        struct stat st;
        if (stat(path.c_str(), &st) != 0) return false;
        
        // Check if file is world-writable (security risk)
        return !(st.st_mode & S_IWOTH);
    }
    
    static std::string sanitize_path(const std::string& path) {
        std::string safe_path;
        for (char c : path) {
            if (std::isalnum(c) || c == '/' || c == '.' || c == '-' || c == '_') {
                safe_path += c;
            }
        }
        return safe_path;
    }
    
    static bool is_safe_command(const std::string& cmd) {
        // Only allow specific safe commands
        return (cmd.find("journalctl") == 0) || 
               (cmd.find("systemctl") == 0) ||
               (cmd.find("uptime") == 0) ||
               (cmd.find("free") == 0) ||
               (cmd.find("uname") == 0);
    }
    
    static void log_security_event(const std::string& event) {
        // Log security events for monitoring
        FILE* log = fopen("/tmp/archlog_security.log", "a");
        if (log) {
            time_t now = time(0);
            fprintf(log, "[%ld] %s: %s\n", now, get_current_user().c_str(), event.c_str());
            fclose(log);
        }
    }
};

#endif