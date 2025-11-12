#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <cstdio>
#include <iomanip>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <cctype>
#include <map>
#include "security.h"
#include "structured_logger.h"

struct LogEntry {
    std::string timestamp;
    std::string level;
    std::string message;
    std::string unit;

    std::string to_user_string() const {
        return ">> **" + level + "** (" + unit + ") | " + timestamp + " | " + message;
    }
};

std::string priority_to_level(int priority) {
    switch (priority) {
        case 0: return "EMERG";
        case 1: return "ALERT";
        case 2: return "CRIT";
        case 3: return "ERROR";
        case 4: return "WARNING";
        case 5: return "NOTICE";
        case 6: return "INFO";
        case 7: return "DEBUG";
        default: return "UNKNOWN";
    }
}

std::string us_to_timestamp(const std::string& us_str) {
    try {
        long long microseconds = std::stoll(us_str);
        time_t seconds = microseconds / 1000000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&seconds), "%Y-%m-%d %H:%M:%S");
        return ss.str();
    } catch (...) {
        return us_str;
    }
}

std::string extract_json_string(const std::string &line, const std::string &key) {
    std::string needle = "\"" + key + "\"";
    size_t p = line.find(needle);
    if (p == std::string::npos) return "";
    p = line.find(':', p + needle.size());
    if (p == std::string::npos) return "";
    
    size_t i = line.find_first_not_of(" \t\n\r", p + 1);
    if (i == std::string::npos || line[i] != '"') return "";
    ++i;
    
    std::string out;
    bool esc = false;
    for (; i < line.size(); ++i) {
        char c = line[i];
        if (esc) { out.push_back(c); esc = false; }
        else if (c == '\\') esc = true;
        else if (c == '"') break;
        else out.push_back(c);
    }
    return out;
}

int extract_json_int(const std::string &line, const std::string &key, int def) {
    std::string needle = "\"" + key + "\"";
    size_t p = line.find(needle);
    if (p == std::string::npos) return def;
    p = line.find(':', p + needle.size());
    if (p == std::string::npos) return def;
    size_t i = line.find_first_not_of(" \t", p + 1);
    if (i == std::string::npos) return def;
    
    size_t j = i;
    while (j < line.size() && (line[j] == '-' || isdigit((unsigned char)line[j]))) ++j;
    if (j == i) return def;
    try {
        return std::stoi(line.substr(i, j - i));
    } catch (...) {
        return def;
    }
}

std::vector<LogEntry> process_journal_logs(int min_priority) {
    std::vector<LogEntry> entries;
    
    const std::string cmd = "journalctl -b -o json -a --no-pager";
    
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        std::cerr << "Error: Could not execute 'journalctl'. Is systemd running and accessible?" << std::endl;
        return entries;
    }

    std::cout << "🚀 **ArchLog Executing:** " << cmd << " | Reading Arch Linux Journal..." << std::endl;

    char buffer[16384];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        std::string line(buffer);
        
        if (line.empty() || line.find('{') == std::string::npos) continue;

        LogEntry entry;

        std::string ts = extract_json_string(line, "__REALTIME_TIMESTAMP");
        if (ts.empty()) ts = extract_json_string(line, "_SOURCE_REALTIME_TIMESTAMP");
        entry.timestamp = ts.empty() ? "N/A" : us_to_timestamp(ts);

        int priority = extract_json_int(line, "PRIORITY", 7);
        entry.level = priority_to_level(priority);

        entry.message = extract_json_string(line, "MESSAGE");
        if (entry.message.empty()) entry.message = "N/A";

        entry.unit = extract_json_string(line, "_SYSTEMD_UNIT");
        if (entry.unit.empty()) entry.unit = extract_json_string(line, "SYSLOG_IDENTIFIER");
        if (entry.unit.empty()) entry.unit = "N/A";

        if (priority <= min_priority) {
            entries.push_back(entry);
        }
    }

    pclose(pipe);
    return entries;
}

bool journalctl_produces_output() {
    const std::string test_cmd = "journalctl -b -o json -a -n 1 --no-pager";
    FILE* pipe = popen(test_cmd.c_str(), "r");
    if (!pipe) return false;
    char buffer[16384];
    bool has = false;
    if (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        std::string line(buffer);
        if (!line.empty() && line.find('{') != std::string::npos) has = true;
    }
    pclose(pipe);
    return has;
}

int level_name_to_priority(const std::string &name) {
    std::string s = name;
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch){ return !std::isspace(ch); }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), s.end());
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c){ return std::toupper(c); });
    
    if (s == "EMERG" || s == "0") return 0;
    if (s == "ALERT" || s == "1") return 1;
    if (s == "CRIT"  || s == "2") return 2;
    if (s == "ERROR" || s == "3") return 3;
    if (s == "WARNING"|| s == "WARN" || s == "4") return 4;
    if (s == "NOTICE"|| s == "5") return 5;
    if (s == "INFO"  || s == "6") return 6;
    if (s == "DEBUG" || s == "7") return 7;
    return -1;
}

std::string truncate_message(const std::string &msg, size_t max_len = 100) {
    if (msg.length() <= max_len) return msg;
    return msg.substr(0, max_len) + "...";
}

void print_entry(const LogEntry &e, bool preview = false, std::ostream& out = std::cout, int entry_num = 0) {
    if (entry_num > 0) {
        out << "[" << std::setfill('0') << std::setw(4) << entry_num << "] ";
    }
    out << "[" << e.timestamp << "] [" << e.level << "] [SYSTEM] " 
        << e.unit << " (/var/log/journal) | ";
    if (preview && &out == &std::cout) {
        out << truncate_message(e.message);
    } else {
        out << e.message;
    }
    out << std::endl;
}

void analyze_arch_logs(int min_priority, bool summary_only=false, int per_unit=5, int tail_count=0, int max_entries=0, bool preview_mode=false) {
    std::vector<LogEntry> all_entries = process_journal_logs(min_priority);
    const std::string LOG_HEADER(60, '=');
    
    auto csv_escape = [](const std::string &s)->std::string {
        std::string out;
        out.push_back('"');
        for (char c: s) {
            if (c == '"') out.push_back('"'), out.push_back('"');
            else out.push_back(c);
        }
        out.push_back('"');
        return out;
    };
    
    std::cout << "\n" << LOG_HEADER << std::endl;
    std::cout << "📈 Arch Linux Journal Analysis Summary" << std::endl;
    std::cout << LOG_HEADER << std::endl;
    
    if (all_entries.empty()) {
        std::cerr << "Error: No log entries were found to process." << std::endl;
        std::cerr << "\nPossible reasons:" << std::endl;
        std::cerr << "  1. Journal access permissions are insufficient" << std::endl;
        std::cerr << "  2. No logs match the current severity filter" << std::endl;
        std::cerr << "  3. System journal is empty for current boot" << std::endl;
        std::cerr << "\nTroubleshooting steps:" << std::endl;

        if (!journalctl_produces_output()) {
            std::cout << "Diagnostic: 'journalctl' produced no JSON output for the current user." << std::endl;
            std::cout << "This usually means your process lacks permission to read the system journal." << std::endl;
            std::cout << "Options to fix: " << std::endl;
            std::cout << "  - Run the program with sudo: sudo ./archlog" << std::endl;
            std::cout << "  - Or add your user to the systemd journal group (if present):" << std::endl;
            std::cout << "      sudo usermod -aG systemd-journal $USER && newgrp systemd-journal" << std::endl;
            std::cout << "  - Or run this specific check yourself to compare behavior:" << std::endl;
            std::cout << "      journalctl -b -o json -a -n 1 --no-pager" << std::endl;
            std::cout << "Then re-run the program after applying the chosen fix." << std::endl;
        } else {
            std::cout << "Diagnostic: 'journalctl' produced output, but no entries matched our filtering criteria." << std::endl;
            std::cout << "You may want to remove the INFO/DEBUG filter or inspect raw journal output." << std::endl;
        }
        return;
    }

    std::map<std::string,int> counts_by_level;
    std::map<std::string,int> counts_by_unit;
    std::map<std::string, std::vector<LogEntry>> entries_by_unit;

    for (const auto &entry : all_entries) {
        counts_by_level[entry.level]++;
        counts_by_unit[entry.unit]++;
        entries_by_unit[entry.unit].push_back(entry);
    }

    std::cout << "\n--- SUMMARY ---" << std::endl;
    std::cout << "Total entries: " << all_entries.size() << std::endl;

    std::cout << "\nBy severity:" << std::endl;
    const std::vector<std::string> sev_order = {"EMERG","ALERT","CRIT","ERROR","WARNING","NOTICE","INFO","DEBUG"};
    for (const auto &s : sev_order) {
        int c = counts_by_level.count(s) ? counts_by_level[s] : 0;
        std::cout << "  " << s << ": " << c << std::endl;
    }

    std::cout << "\nTop units by count:" << std::endl;
    std::vector<std::pair<std::string,int>> units;
    for (auto &p : counts_by_unit) units.emplace_back(p.first, p.second);
    std::sort(units.begin(), units.end(), [](auto &a, auto &b){ return a.second > b.second; });
    for (size_t i = 0; i < units.size() && i < 20; ++i) {
        std::cout << "  " << units[i].first << ": " << units[i].second << std::endl;
    }

    if (summary_only) {
        std::cout << "\n(Showing summary followed by ALL log messages below)" << std::endl;
        std::cout << "\n--- LOG ENTRIES (chronological) ---" << std::endl;
        size_t total = all_entries.size();
        size_t start = 0;
        if (max_entries > 0 && (size_t)max_entries < total) start = total - (size_t)max_entries;
        for (size_t i = start; i < total; ++i) {
            print_entry(all_entries[i], preview_mode, std::cout, (int)(i - start + 1));
            std::cout << std::endl;
        }
        return;
    }

    if (tail_count > 0) {
        std::cout << "\n--- LAST " << tail_count << " ENTRIES (most recent first) ---" << std::endl;
        int printed = 0;
        for (auto it = all_entries.rbegin(); it != all_entries.rend() && printed < tail_count; ++it, ++printed) {
            print_entry(*it, preview_mode, std::cout, printed + 1);
            std::cout << std::endl;
        }
        return;
    }

    std::cout << "\n--- RECENT ENTRIES BY TOP UNITS (showing up to " << per_unit << " each) ---" << std::endl;
    for (size_t ui = 0; ui < units.size() && ui < 20; ++ui) {
        const std::string &unit = units[ui].first;
        auto &vec = entries_by_unit[unit];
        std::cout << "\n================================================" << std::endl;
        std::cout << "Log: " << unit << " (" << vec.size() << " entries)" << std::endl;
        std::cout << "================================================" << std::endl;
        int printed = 0;
        for (auto it = vec.rbegin(); it != vec.rend() && printed < per_unit; ++it, ++printed) {
            print_entry(*it, preview_mode, std::cout, printed + 1);
        }
    }

    const char *csvenv = std::getenv("TEST_CSV");
    if (csvenv && std::string(csvenv) == "1") {
        std::cout << "\n--- CSV TABLE (timestamp,level,unit,message) ---" << std::endl;
        std::cout << "timestamp,level,unit,message" << std::endl;
        for (const auto &it : all_entries) {
            std::cout << csv_escape(it.timestamp) << "," << csv_escape(it.level) << "," << csv_escape(it.unit) << "," << csv_escape(it.message) << std::endl;
        }
    }
}

int main(int argc, char** argv) {
    StructuredLogger::initialize();
    StructuredLogger::system("archlog", "/usr/bin", "ArchLog CLI starting");
    
    try {
        int min_priority = 5;
        bool summary_only = false;
        int per_unit = 5;
        int tail_count = 0;
        bool want_csv = false;
        int max_entries = 0;
        bool preview_mode = false;

        for (int i = 1; i < argc; ++i) {
            std::string a = argv[i];
            if (a.rfind("--min-level=", 0) == 0) {
                std::string v = SecurityValidator::sanitize_severity(a.substr(std::string("--min-level=").size()));
                int p = level_name_to_priority(v);
                if (p < 0) {
                    std::cerr << "Error: Unrecognized severity level: " << v << std::endl;
                    std::cerr << "Valid levels: EMERG, ALERT, CRIT, ERROR, WARNING, NOTICE, INFO, DEBUG" << std::endl;
                    return 1;
                }
                min_priority = p;
            } else if (a == "-m") {
                if (i + 1 >= argc) {
                    std::cerr << "-m requires an argument (e.g. -m INFO)" << std::endl;
                    return 1;
                }
                std::string v = SecurityValidator::sanitize_severity(argv[++i]);
                int p = level_name_to_priority(v);
                if (p < 0) { std::cerr << "Unrecognized min-level value: " << v << std::endl; return 1; }
                min_priority = p;
            } else if (a == "--no-filter") {
                min_priority = 7;
            } else if (a == "--summary") {
                summary_only = true;
            } else if (a.rfind("--per-unit=", 0) == 0) {
                std::string v = a.substr(std::string("--per-unit=").size());
                try { 
                    per_unit = std::stoi(v); 
                    if (per_unit < 0) throw std::invalid_argument("negative value");
                } catch(...) { 
                    std::cerr << "Error: Invalid --per-unit value: " << v << " (must be positive integer)" << std::endl; 
                    return 1; 
                }
            } else if (a.rfind("--tail=", 0) == 0) {
                std::string v = a.substr(std::string("--tail=").size());
                try { 
                    tail_count = std::stoi(v); 
                    if (tail_count < 0) throw std::invalid_argument("negative value");
                } catch(...) { 
                    std::cerr << "Error: Invalid --tail value: " << v << " (must be positive integer)" << std::endl; 
                    return 1; 
                }
            } else if (a == "--csv" || a == "--table") {
                want_csv = true;
            } else if (a == "--preview") {
                preview_mode = true;
            } else if (a.rfind("--max-entries=", 0) == 0) {
                std::string v = a.substr(std::string("--max-entries=").size());
                try { 
                    max_entries = std::stoi(v); 
                    if (max_entries < 0) throw std::invalid_argument("negative value");
                } catch(...) { 
                    std::cerr << "Error: Invalid --max-entries value: " << v << " (must be positive integer)" << std::endl; 
                    return 1; 
                }
            } else if (a.rfind("--unit=", 0) == 0) {
                std::string v = a.substr(std::string("--unit=").size());
                // Filter by specific unit (not implemented in this minimal version)
            } else if (a.rfind("--since=", 0) == 0) {
                std::string v = a.substr(std::string("--since=").size());
                // Time filtering (not implemented in this minimal version)
            } else if (a == "-f" || a == "--follow") {
                // Follow mode (not implemented in this minimal version)
            } else if (a == "--help" || a == "-h") {
                std::cout << "ArchVault - Advanced System Log Analyzer" << std::endl;
                std::cout << "Usage: " << argv[0] << " [options]" << std::endl;
                std::cout << "\nFilter options:" << std::endl;
                std::cout << "  --min-level=LEVEL|-m LEVEL  Minimum severity level to show" << std::endl;
                std::cout << "  --no-filter                 Show all messages (including INFO/DEBUG)" << std::endl;
                std::cout << "\nOutput options:" << std::endl;
                std::cout << "  --summary                   Show analysis summary and entries" << std::endl;
                std::cout << "  --tail=N                    Show only last N entries" << std::endl;
                std::cout << "  --max-entries=N            Limit entries shown with --summary" << std::endl;
                std::cout << "  --csv                      Output as CSV (timestamp,level,unit,message)" << std::endl;
                std::cout << "\nSeverity levels (most to least severe):" << std::endl;
                std::cout << "  EMERG(0)   System is unusable" << std::endl;
                std::cout << "  ALERT(1)   Action must be taken immediately" << std::endl;
                std::cout << "  CRIT(2)    Critical conditions" << std::endl;
                std::cout << "  ERROR(3)   Error conditions" << std::endl;
                std::cout << "  WARNING(4) Warning conditions" << std::endl;
                std::cout << "  NOTICE(5)  Normal but significant (default minimum)" << std::endl;
                std::cout << "  INFO(6)    Informational" << std::endl;
                std::cout << "  DEBUG(7)   Debug-level messages" << std::endl;
                std::cout << "\nExamples:" << std::endl;
                std::cout << "  " << argv[0] << " --summary --no-filter     Show summary with all severities" << std::endl;
                std::cout << "  " << argv[0] << " -m ERROR --tail=50       Show last 50 errors or higher" << std::endl;
                std::cout << "  " << argv[0] << " --summary --max-entries=200  Show summary with at most 200 entries" << std::endl;
                return 0;
            } else {
                std::cerr << "Unknown argument: " << a << std::endl;
                std::cerr << "Use --help for usage information." << std::endl;
                return 1;
            }
        }

        if (want_csv) setenv("TEST_CSV", "1", 1);

        analyze_arch_logs(min_priority, summary_only, per_unit, tail_count, max_entries, preview_mode);
        
        if (want_csv) unsetenv("TEST_CSV");
    } catch (const std::exception& e) {
        std::cerr << "\nAn unexpected error occurred: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}