#ifndef SECURITY_H
#define SECURITY_H

#include <string>
#include <cctype>

class SecurityValidator {
public:
    static std::string sanitize_time_input(const std::string& input) {
        std::string safe;
        for (char c : input) {
            if (std::isalnum(c) || c == ' ' || c == ':' || c == '-' || c == '+') {
                safe += c;
            }
        }
        return safe;
    }
    
    static std::string sanitize_unit_input(const std::string& input) {
        std::string safe;
        for (char c : input) {
            if (std::isalnum(c) || c == '.' || c == '-' || c == '_' || c == '@') {
                safe += c;
            }
        }
        return safe;
    }
    
    static bool is_valid_number(const std::string& input) {
        if (input.empty()) return false;
        for (char c : input) {
            if (!std::isdigit(c)) return false;
        }
        return true;
    }
    
    static std::string sanitize_severity(const std::string& input) {
        std::string safe;
        for (char c : input) {
            if (std::isalpha(c)) {
                safe += std::toupper(c);
            }
        }
        return safe;
    }
};

#endif