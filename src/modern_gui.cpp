#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <iostream>
#include <string>
#include <vector>
#include <cstdio>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <chrono>
#include <cstdlib>
#include "security.h"
#include "arch_features.h"
#include "enhanced_security.h"
#include "quick_actions.h"
#include "hardware_monitor.h"
#include "structured_logger.h"

class ModernArchLogGUI {
private:
    GtkWidget *window;
    GtkWidget *header_bar;
    GtkWidget *main_stack;
    GtkWidget *sidebar;
    GtkWidget *text_view;
    GtkTextBuffer *buffer;
    GtkWidget *combo_level;
    GtkWidget *combo_unit;
    GtkWidget *entry_tail;
    GtkWidget *entry_since;
    GtkWidget *check_summary;
    GtkWidget *check_csv;
    GtkWidget *check_watch;
    GtkWidget *progress_bar;
    GtkWidget *status_label;
    GtkWidget *hardware_panel;
    GtkWidget *cpu_progress;
    GtkWidget *memory_progress;
    GtkWidget *disk_progress;
    GtkWidget *cpu_temp_label;
    GtkWidget *gpu_temp_label;
    GtkWidget *gpu_progress;
    GtkWidget *network_label;
    std::mutex buffer_mutex;
    std::atomic<bool> is_running{false};
    std::atomic<bool> monitor_running{false};
    bool initialized{false};

public:
    ModernArchLogGUI() {
        StructuredLogger::initialize();
        StructuredLogger::user_action("GUI Initialization Started");
        
        // Try to initialize GTK with proper error handling
        int argc = 0;
        char** argv = nullptr;
        
        if (!gtk_init_check(&argc, &argv)) {
            // Fallback: set minimal display if none exists
            if (!getenv("DISPLAY") && !getenv("WAYLAND_DISPLAY")) {
                setenv("DISPLAY", ":0", 0);
                if (!gtk_init_check(&argc, &argv)) {
                    StructuredLogger::error("gui_init", "/gui", "GTK initialization failed - running in headless mode");
                    return; // Don't throw, just return
                }
            } else {
                StructuredLogger::error("gui_init", "/gui", "GTK initialization failed");
                return;
            }
        }
        
        try {
            create_window();
            create_header_bar();
            create_main_layout();
            apply_system_theme();
            
            gtk_widget_show_all(window);
            refresh_units();
            
            StructuredLogger::info("gui_init", "/gui", "GUI initialized successfully with dark theme");
            initialized = true;
        } catch (const std::exception& e) {
            StructuredLogger::error("gui_init", "/gui", std::string("GUI setup failed: ") + e.what());
            initialized = false;
        }
    }
    
    bool is_initialized() const { return initialized; }

    void create_window() {
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(window), "ArchVault - System Monitor");
        gtk_window_set_default_size(GTK_WINDOW(window), 1400, 900);
        gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
        gtk_window_set_icon_name(GTK_WINDOW(window), "utilities-system-monitor");
        
        // Force dark theme
        GtkSettings *settings = gtk_settings_get_default();
        g_object_set(settings, "gtk-application-prefer-dark-theme", TRUE, NULL);
        g_object_set(settings, "gtk-theme-name", "Adwaita-dark", NULL);
        
        // Set environment for consistent dark theme
        setenv("GTK_THEME", "Adwaita:dark", 1);
        g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    }

    void create_header_bar() {
        header_bar = gtk_header_bar_new();
        gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(header_bar), TRUE);
        gtk_header_bar_set_title(GTK_HEADER_BAR(header_bar), "ArchVault");
        gtk_header_bar_set_subtitle(GTK_HEADER_BAR(header_bar), "Advanced System Log Analyzer v2.0");
        gtk_window_set_titlebar(GTK_WINDOW(window), header_bar);

        // Header buttons with enhanced styling
        GtkWidget *btn_run = gtk_button_new_with_label("▶ RUN");
        gtk_widget_set_tooltip_text(btn_run, "Run Log Analysis");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_run), "suggested-action");
        g_signal_connect(btn_run, "clicked", G_CALLBACK(on_analyze_clicked), this);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), btn_run);

        GtkWidget *btn_clear = gtk_button_new_with_label("🗑 CLEAR");
        gtk_widget_set_tooltip_text(btn_clear, "Clear Output");
        gtk_style_context_add_class(gtk_widget_get_style_context(btn_clear), "destructive-action");
        g_signal_connect(btn_clear, "clicked", G_CALLBACK(on_clear_clicked), this);
        gtk_header_bar_pack_start(GTK_HEADER_BAR(header_bar), btn_clear);

        GtkWidget *btn_save = gtk_button_new_with_label("💾 SAVE");
        gtk_widget_set_tooltip_text(btn_save, "Save Output");
        g_signal_connect(btn_save, "clicked", G_CALLBACK(on_save_clicked), this);
        gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), btn_save);
        
        GtkWidget *btn_export = gtk_button_new_with_label("📤 EXPORT");
        gtk_widget_set_tooltip_text(btn_export, "Export Analysis");
        g_signal_connect(btn_export, "clicked", G_CALLBACK(on_export_clicked), this);
        gtk_header_bar_pack_end(GTK_HEADER_BAR(header_bar), btn_export);
    }

    void create_main_layout() {
        GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
        gtk_container_add(GTK_CONTAINER(window), main_box);

        // Sidebar
        create_sidebar();
        gtk_box_pack_start(GTK_BOX(main_box), sidebar, FALSE, FALSE, 0);

        // Main content area
        GtkWidget *content_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_box_pack_start(GTK_BOX(main_box), content_box, TRUE, TRUE, 0);

        // Enhanced progress bar with status
        GtkWidget *progress_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_start(progress_box, 10);
        gtk_widget_set_margin_end(progress_box, 10);
        gtk_widget_set_margin_top(progress_box, 5);
        gtk_widget_set_margin_bottom(progress_box, 5);
        
        progress_bar = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(progress_bar), TRUE);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Ready");
        gtk_box_pack_start(GTK_BOX(progress_box), progress_bar, TRUE, TRUE, 0);
        
        GtkWidget *stop_btn = gtk_button_new_with_label("⏹ STOP");
        gtk_widget_set_tooltip_text(stop_btn, "Stop Analysis");
        gtk_style_context_add_class(gtk_widget_get_style_context(stop_btn), "destructive-action");
        g_signal_connect(stop_btn, "clicked", G_CALLBACK(on_stop_clicked), this);
        gtk_box_pack_start(GTK_BOX(progress_box), stop_btn, FALSE, FALSE, 0);
        
        gtk_box_pack_start(GTK_BOX(content_box), progress_box, FALSE, FALSE, 0);

        // Text view with modern styling
        GtkWidget *scrolled = gtk_scrolled_window_new(NULL, NULL);
        gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
        gtk_widget_set_size_request(scrolled, -1, 600);

        text_view = gtk_text_view_new();
        gtk_text_view_set_editable(GTK_TEXT_VIEW(text_view), FALSE);
        gtk_text_view_set_monospace(GTK_TEXT_VIEW(text_view), TRUE);
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(text_view), GTK_WRAP_WORD);
        buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view));
        gtk_container_add(GTK_CONTAINER(scrolled), text_view);
        gtk_box_pack_start(GTK_BOX(content_box), scrolled, TRUE, TRUE, 0);

        // Enhanced status bar
        GtkWidget *status_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        gtk_widget_set_margin_start(status_box, 10);
        gtk_widget_set_margin_end(status_box, 10);
        gtk_widget_set_margin_top(status_box, 5);
        gtk_widget_set_margin_bottom(status_box, 10);
        
        status_label = gtk_label_new("🚀 Ready - ArchVault System Monitor");
        gtk_widget_set_halign(status_label, GTK_ALIGN_START);
        gtk_style_context_add_class(gtk_widget_get_style_context(status_label), "dim-label");
        gtk_box_pack_start(GTK_BOX(status_box), status_label, TRUE, TRUE, 0);
        
        GtkWidget *time_label = gtk_label_new("");
        gtk_widget_set_halign(time_label, GTK_ALIGN_END);
        gtk_style_context_add_class(gtk_widget_get_style_context(time_label), "dim-label");
        gtk_box_pack_start(GTK_BOX(status_box), time_label, FALSE, FALSE, 0);
        
        // Update time every second
        g_timeout_add_seconds(1, update_time_label, time_label);
        
        gtk_box_pack_start(GTK_BOX(content_box), status_box, FALSE, FALSE, 0);
    }

    void create_sidebar() {
        sidebar = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
        gtk_widget_set_size_request(sidebar, 350, -1);
        
        // Professional branding header
        GtkWidget *brand_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
        gtk_widget_set_margin_start(brand_box, 16);
        gtk_widget_set_margin_end(brand_box, 16);
        gtk_widget_set_margin_top(brand_box, 16);
        gtk_widget_set_margin_bottom(brand_box, 12);
        
        GtkWidget *brand_label = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(brand_label), "<span size='large' weight='bold'>ArchVault</span>\n<span size='small' alpha='70%'>System Monitor</span>");
        gtk_widget_set_halign(brand_label, GTK_ALIGN_START);
        gtk_box_pack_start(GTK_BOX(brand_box), brand_label, FALSE, FALSE, 0);
        
        gtk_box_pack_start(GTK_BOX(sidebar), brand_box, FALSE, FALSE, 0);
        
        GtkWidget *separator = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
        gtk_widget_set_margin_start(separator, 16);
        gtk_widget_set_margin_end(separator, 16);
        gtk_box_pack_start(GTK_BOX(sidebar), separator, FALSE, FALSE, 0);
        gtk_widget_set_margin_start(sidebar, 10);
        gtk_widget_set_margin_end(sidebar, 10);
        gtk_widget_set_margin_top(sidebar, 10);
        gtk_widget_set_margin_bottom(sidebar, 10);

        // Filters section
        GtkWidget *filters_frame = gtk_frame_new("Filters");
        gtk_box_pack_start(GTK_BOX(sidebar), filters_frame, FALSE, FALSE, 0);

        GtkWidget *filters_grid = gtk_grid_new();
        gtk_grid_set_row_spacing(GTK_GRID(filters_grid), 8);
        gtk_grid_set_column_spacing(GTK_GRID(filters_grid), 8);
        gtk_widget_set_margin_start(filters_grid, 10);
        gtk_widget_set_margin_end(filters_grid, 10);
        gtk_widget_set_margin_top(filters_grid, 10);
        gtk_widget_set_margin_bottom(filters_grid, 10);
        gtk_container_add(GTK_CONTAINER(filters_frame), filters_grid);

        // Severity
        gtk_grid_attach(GTK_GRID(filters_grid), gtk_label_new("Severity:"), 0, 0, 1, 1);
        combo_level = gtk_combo_box_text_new();
        const char* levels[] = {"ALL", "EMERG", "ALERT", "CRIT", "ERROR", "WARNING", "NOTICE", "INFO", "DEBUG"};
        for (const char* level : levels) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_level), level);
        }
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_level), 0);
        gtk_grid_attach(GTK_GRID(filters_grid), combo_level, 0, 1, 1, 1);

        // Unit
        gtk_grid_attach(GTK_GRID(filters_grid), gtk_label_new("Service:"), 0, 2, 1, 1);
        combo_unit = gtk_combo_box_text_new();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_unit), "All Services");
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_unit), 0);
        gtk_grid_attach(GTK_GRID(filters_grid), combo_unit, 0, 3, 1, 1);

        // Time
        gtk_grid_attach(GTK_GRID(filters_grid), gtk_label_new("Time Period:"), 0, 4, 1, 1);
        entry_since = gtk_combo_box_text_new_with_entry();
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_since), "1 hour ago");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_since), "");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_since), "today");
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(entry_since), "yesterday");
        gtk_combo_box_set_active(GTK_COMBO_BOX(entry_since), 0);
        gtk_grid_attach(GTK_GRID(filters_grid), entry_since, 0, 5, 1, 1);

        // Entries
        gtk_grid_attach(GTK_GRID(filters_grid), gtk_label_new("Max Entries:"), 0, 6, 1, 1);
        entry_tail = gtk_entry_new();
        gtk_entry_set_text(GTK_ENTRY(entry_tail), "100");
        gtk_grid_attach(GTK_GRID(filters_grid), entry_tail, 0, 7, 1, 1);

        // Options
        GtkWidget *options_frame = gtk_frame_new("Options");
        gtk_box_pack_start(GTK_BOX(sidebar), options_frame, FALSE, FALSE, 0);

        GtkWidget *options_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(options_box, 10);
        gtk_widget_set_margin_end(options_box, 10);
        gtk_widget_set_margin_top(options_box, 10);
        gtk_widget_set_margin_bottom(options_box, 10);
        gtk_container_add(GTK_CONTAINER(options_frame), options_box);

        check_summary = gtk_check_button_new_with_label("Summary Analysis");
        gtk_box_pack_start(GTK_BOX(options_box), check_summary, FALSE, FALSE, 0);

        check_csv = gtk_check_button_new_with_label("CSV Export");
        gtk_box_pack_start(GTK_BOX(options_box), check_csv, FALSE, FALSE, 0);

        check_watch = gtk_check_button_new_with_label("Real-time Watch");
        gtk_box_pack_start(GTK_BOX(options_box), check_watch, FALSE, FALSE, 0);

        // Quick Actions
        GtkWidget *quick_frame = gtk_frame_new("Quick Actions");
        gtk_box_pack_start(GTK_BOX(sidebar), quick_frame, FALSE, FALSE, 0);

        GtkWidget *quick_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(quick_box, 10);
        gtk_widget_set_margin_end(quick_box, 10);
        gtk_widget_set_margin_top(quick_box, 10);
        gtk_widget_set_margin_bottom(quick_box, 10);
        gtk_container_add(GTK_CONTAINER(quick_frame), quick_box);

        auto quick_filters = QuickActions::get_quick_filters();
        for (size_t i = 0; i < quick_filters.size(); ++i) {
            GtkWidget *btn = gtk_button_new_with_label(quick_filters[i].first.c_str());
            g_object_set_data(G_OBJECT(btn), "gui_instance", this);
            g_object_set_data(G_OBJECT(btn), "action_index", GINT_TO_POINTER(i));
            g_signal_connect(btn, "clicked", G_CALLBACK(on_quick_action_clicked), btn);
            gtk_box_pack_start(GTK_BOX(quick_box), btn, FALSE, FALSE, 0);
        }
        
        GtkWidget *processes_btn = gtk_button_new_with_label("⚡ Top Processes");
        g_signal_connect(processes_btn, "clicked", G_CALLBACK(on_processes_clicked), this);
        gtk_box_pack_start(GTK_BOX(quick_box), processes_btn, FALSE, FALSE, 0);
        
        GtkWidget *network_btn = gtk_button_new_with_label("🌐 Network Status");
        g_signal_connect(network_btn, "clicked", G_CALLBACK(on_network_clicked), this);
        gtk_box_pack_start(GTK_BOX(quick_box), network_btn, FALSE, FALSE, 0);
        
        GtkWidget *services_btn = gtk_button_new_with_label("🔧 Services");
        g_signal_connect(services_btn, "clicked", G_CALLBACK(on_services_clicked), this);
        gtk_box_pack_start(GTK_BOX(quick_box), services_btn, FALSE, FALSE, 0);
        
        GtkWidget *disk_btn = gtk_button_new_with_label("💾 Disk Info");
        g_signal_connect(disk_btn, "clicked", G_CALLBACK(on_disk_clicked), this);
        gtk_box_pack_start(GTK_BOX(quick_box), disk_btn, FALSE, FALSE, 0);

        // System Info
        GtkWidget *sysinfo_btn = gtk_button_new_with_label("🖥️ System Info");
        g_signal_connect(sysinfo_btn, "clicked", G_CALLBACK(on_sysinfo_clicked), this);
        gtk_box_pack_start(GTK_BOX(quick_box), sysinfo_btn, FALSE, FALSE, 0);

        GtkWidget *pacman_btn = gtk_button_new_with_label("📦 Pacman Logs");
        g_signal_connect(pacman_btn, "clicked", G_CALLBACK(on_pacman_clicked), this);
        gtk_box_pack_start(GTK_BOX(quick_box), pacman_btn, FALSE, FALSE, 0);
        
        // New Features
        GtkWidget *security_btn = gtk_button_new_with_label("🔒 Security Scan");
        g_signal_connect(security_btn, "clicked", G_CALLBACK(on_security_scan_clicked), this);
        gtk_box_pack_start(GTK_BOX(quick_box), security_btn, FALSE, FALSE, 0);
        
        GtkWidget *performance_btn = gtk_button_new_with_label("⚡ Performance");
        g_signal_connect(performance_btn, "clicked", G_CALLBACK(on_performance_clicked), this);
        gtk_box_pack_start(GTK_BOX(quick_box), performance_btn, FALSE, FALSE, 0);
        
        GtkWidget *logs_btn = gtk_button_new_with_label("📋 Export Logs");
        g_signal_connect(logs_btn, "clicked", G_CALLBACK(on_export_logs_clicked), this);
        gtk_box_pack_start(GTK_BOX(quick_box), logs_btn, FALSE, FALSE, 0);
        
        // Hardware Monitor
        create_hardware_panel();
        gtk_box_pack_start(GTK_BOX(sidebar), hardware_panel, FALSE, FALSE, 0);
    }
    
    void create_hardware_panel() {
        hardware_panel = gtk_frame_new("Hardware Monitor");
        
        GtkWidget *hw_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_widget_set_margin_start(hw_box, 10);
        gtk_widget_set_margin_end(hw_box, 10);
        gtk_widget_set_margin_top(hw_box, 10);
        gtk_widget_set_margin_bottom(hw_box, 10);
        gtk_container_add(GTK_CONTAINER(hardware_panel), hw_box);
        
        // CPU Usage
        gtk_box_pack_start(GTK_BOX(hw_box), gtk_label_new("CPU Usage:"), FALSE, FALSE, 0);
        cpu_progress = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(cpu_progress), TRUE);
        gtk_box_pack_start(GTK_BOX(hw_box), cpu_progress, FALSE, FALSE, 0);
        
        // Memory Usage
        gtk_box_pack_start(GTK_BOX(hw_box), gtk_label_new("Memory Usage:"), FALSE, FALSE, 0);
        memory_progress = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(memory_progress), TRUE);
        gtk_box_pack_start(GTK_BOX(hw_box), memory_progress, FALSE, FALSE, 0);
        
        // Disk Usage
        gtk_box_pack_start(GTK_BOX(hw_box), gtk_label_new("Disk Usage:"), FALSE, FALSE, 0);
        disk_progress = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(disk_progress), TRUE);
        gtk_box_pack_start(GTK_BOX(hw_box), disk_progress, FALSE, FALSE, 0);
        
        // GPU Usage
        gtk_box_pack_start(GTK_BOX(hw_box), gtk_label_new("GPU Usage:"), FALSE, FALSE, 0);
        gpu_progress = gtk_progress_bar_new();
        gtk_progress_bar_set_show_text(GTK_PROGRESS_BAR(gpu_progress), TRUE);
        gtk_box_pack_start(GTK_BOX(hw_box), gpu_progress, FALSE, FALSE, 0);
        
        // Temperature
        cpu_temp_label = gtk_label_new("CPU: --°C");
        gtk_box_pack_start(GTK_BOX(hw_box), cpu_temp_label, FALSE, FALSE, 0);
        
        gpu_temp_label = gtk_label_new("GPU: --°C");
        gtk_box_pack_start(GTK_BOX(hw_box), gpu_temp_label, FALSE, FALSE, 0);
        
        // Network
        network_label = gtk_label_new("Network: -- KB/s");
        gtk_box_pack_start(GTK_BOX(hw_box), network_label, FALSE, FALSE, 0);
        
        // System Load
        GtkWidget *load_label = gtk_label_new("Load: 0.0");
        g_object_set_data(G_OBJECT(hw_box), "load_label", load_label);
        gtk_box_pack_start(GTK_BOX(hw_box), load_label, FALSE, FALSE, 0);
        
        // Active Connections
        GtkWidget *conn_label = gtk_label_new("Connections: 0");
        g_object_set_data(G_OBJECT(hw_box), "conn_label", conn_label);
        gtk_box_pack_start(GTK_BOX(hw_box), conn_label, FALSE, FALSE, 0);
        
        // Start monitoring
        start_hardware_monitoring();
    }

    void apply_system_theme() {
        bool use_dark_theme = detect_system_dark_mode();
        GtkCssProvider *provider = gtk_css_provider_new();
        const char *css = use_dark_theme ? get_dark_theme_css() : get_light_theme_css();
        gtk_css_provider_load_from_data(provider, css, -1, NULL);
        gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
                                                 GTK_STYLE_PROVIDER(provider),
                                                 GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    
    bool detect_system_dark_mode() {
        GtkSettings *settings = gtk_settings_get_default();
        gboolean prefer_dark = FALSE;
        g_object_get(settings, "gtk-application-prefer-dark-theme", &prefer_dark, NULL);
        if (prefer_dark) return true;
        
        gchar *theme_name = NULL;
        g_object_get(settings, "gtk-theme-name", &theme_name, NULL);
        if (theme_name) {
            std::string theme(theme_name);
            g_free(theme_name);
            if (theme.find("dark") != std::string::npos || theme.find("Dark") != std::string::npos) {
                return true;
            }
        }
        
        const char* theme_env = getenv("GTK_THEME");
        if (theme_env && (strstr(theme_env, "dark") || strstr(theme_env, "Dark"))) {
            return true;
        }
        return false;
    }
    
    const char* get_dark_theme_css() {
        return 
            "@define-color primary_color #00d4ff;"
            "@define-color secondary_color #0099cc;"
            "@define-color success_color #00ff88;"
            "@define-color danger_color #ff4444;"
            "@define-color warning_color #ffaa00;"
            "@define-color bg_black #000000;"
            "@define-color bg_dark #0d1117;"
            "@define-color bg_card #161b22;"
            "@define-color border_dark #30363d;"
            "@define-color border_color #21262d;"
            "@define-color text_white #f0f6fc;"
            "@define-color text_primary #e6edf3;"
            "@define-color text_secondary #7d8590;"
            "@define-color text_muted #656d76;"
            "@define-color card_color #21262d;"
            
            "window { "
            "  background-color: #000000; "
            "  color: #f0f6fc; "
            "  font-family: monospace; "
            "  font-size: 13px; "
            "}"
            
            "headerbar { "
            "  background: #0d1117; "
            "  color: #f0f6fc; "
            "  border-bottom: 1px solid #30363d; "
            "  min-height: 48px; "
            "}"
            
            "headerbar button { "
            "  margin: 4px 6px; "
            "  min-width: 80px; "
            "  min-height: 36px; "
            "  font-weight: 500; "
            "  font-size: 12px; "
            "  border-radius: 8px; "
            "  border: 1px solid #21262d; "
            "  background: #21262d; "
            "  color: #e6edf3; "
            "}"
            
            "button.suggested-action { "
            "  background: #00d4ff; "
            "  color: white; "
            "  border: 1px solid #00d4ff; "
            "  font-weight: 600; "
            "}"
            
            "button.suggested-action:hover { "
            "  background: #0099cc; "
            "  border-color: #0099cc; "
            "}"
            
            "button.destructive-action { "
            "  background: #ff4444; "
            "  color: white; "
            "  border: 1px solid #ff4444; "
            "}"
            
            "frame { "
            "  border: 1px solid #30363d; "
            "  background: #161b22; "
            "  border-radius: 8px; "
            "  margin: 8px; "
            "}"
            
            "frame > label { "
            "  color: #00d4ff; "
            "  font-weight: 600; "
            "  font-size: 11px; "
            "  padding: 12px 16px 8px 16px; "
            "}"
            
            "textview { "
            "  background-color: #000000; "
            "  color: #f0f6fc; "
            "  font-family: monospace; "
            "  font-size: 12px; "
            "  border-radius: 6px; "
            "  border: 1px solid #30363d; "
            "  padding: 12px; "
            "}"
            
            "button { "
            "  background: #161b22; "
            "  color: #f0f6fc; "
            "  border: 1px solid #30363d; "
            "  border-radius: 6px; "
            "  padding: 8px 16px; "
            "  font-weight: 500; "
            "}"
            
            "button:hover { "
            "  background: #0d1117; "
            "  border-color: #00d4ff; "
            "}"
            
            "entry { "
            "  background-color: #0d1117; "
            "  color: #f0f6fc; "
            "  border: 1px solid #30363d; "
            "  border-radius: 6px; "
            "  padding: 8px 12px; "
            "  font-size: 12px; "
            "}"
            
            "entry:focus { "
            "  border-color: #00d4ff; "
            "}"
            
            "progressbar { "
            "  background-color: #30363d; "
            "  border-radius: 6px; "
            "  min-height: 8px; "
            "}"
            
            "progressbar progress { "
            "  background: #00d4ff; "
            "  border-radius: 6px; "
            "}"
            
            "separator { "
            "  background-color: #21262d; "
            "  min-height: 1px; "
            "}"
            
            "label.dim-label { "
            "  color: #656d76; "
            "  font-size: 11px; "
            "  font-weight: 400; "
            "}"
            
            "scrollbar { "
            "  background-color: transparent; "
            "  border-radius: 6px; "
            "}"
            
            "scrollbar slider { "
            "  background-color: #656d76; "
            "  border-radius: 4px; "
            "  min-width: 6px; "
            "}"
            
            "scrollbar slider:hover { "
            "  background-color: #7d8590; "
            "}"
            
            "combobox button { "
            "  background: #21262d; "
            "  border: 1px solid #21262d; "
            "  border-radius: 6px; "
            "}"
            
            "checkbutton { "
            "  color: #f0f6fc; "
            "}"
            
            "checkbutton:checked { "
            "  color: #00d4ff; "
            "}"
            
            "label { "
            "  color: #f0f6fc; "
            "}"
            
            "combobox button { "
            "  background: #161b22; "
            "  color: #f0f6fc; "
            "  border: 1px solid #30363d; "
            "}"
            
            "scrollbar { "
            "  background-color: #0d1117; "
            "}"
            
            "scrollbar slider { "
            "  background-color: #656d76; "
            "}";
    }
    
    const char* get_light_theme_css() {
        return
            "@define-color primary_color #2563eb;"
            "@define-color bg_white #ffffff;"
            "@define-color bg_light #f8fafc;"
            "@define-color text_dark #0f172a;"
            "window { background-color: @bg_white; color: @text_dark; }"
            "headerbar { background: @bg_light; color: @text_dark; }"
            "textview { background-color: @bg_white; color: @text_dark; }"
            "button { background: @bg_light; color: @text_dark; }"
            "entry { background-color: @bg_white; color: @text_dark; }"
            "label { color: @text_dark; }";
    }

    void run() {
        gtk_main();
    }

    // Static callbacks
    static void on_analyze_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->analyze_logs();
    }

    static void on_clear_clicked(GtkWidget*, gpointer data) {
        ModernArchLogGUI *gui = static_cast<ModernArchLogGUI*>(data);
        gtk_text_buffer_set_text(gui->buffer, "", -1);
        gtk_label_set_text(GTK_LABEL(gui->status_label), "Output cleared");
    }

    static void on_save_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->save_output();
    }
    
    static void on_export_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->export_analysis();
    }
    
    static void on_stop_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->stop_analysis();
    }
    
    static gboolean update_time_label(gpointer data) {
        GtkWidget* label = GTK_WIDGET(data);
        time_t now = time(0);
        struct tm* tm_info = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
        gtk_label_set_text(GTK_LABEL(label), time_str);
        return G_SOURCE_CONTINUE;
    }

    static void on_sysinfo_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->show_system_info();
    }

    static void on_pacman_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->show_pacman_logs();
    }
    
    static void on_processes_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->show_top_processes();
    }
    
    static void on_network_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->show_network_status();
    }
    
    static void on_services_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->show_services_status();
    }
    
    static void on_disk_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->show_disk_info();
    }
    
    static void on_security_scan_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->run_security_scan();
    }
    
    static void on_performance_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->show_performance_analysis();
    }
    
    static void on_export_logs_clicked(GtkWidget*, gpointer data) {
        static_cast<ModernArchLogGUI*>(data)->export_structured_logs();
    }

    static void on_quick_action_clicked(GtkWidget* button, gpointer) {
        ModernArchLogGUI* gui = static_cast<ModernArchLogGUI*>(g_object_get_data(G_OBJECT(button), "gui_instance"));
        int index = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "action_index"));
        gui->execute_quick_action(index);
    }
    
    void start_hardware_monitoring() {
        monitor_running.store(true);
        std::thread([this]() {
            while (monitor_running.load()) {
                auto stats = HardwareMonitor::get_current_stats();
                
                g_idle_add([](gpointer data) -> gboolean {
                    auto pair = static_cast<std::pair<ModernArchLogGUI*, HardwareStats>*>(data);
                    pair->first->update_hardware_display(pair->second);
                    delete pair;
                    return G_SOURCE_REMOVE;
                }, new std::pair<ModernArchLogGUI*, HardwareStats>(this, stats));
                
                std::this_thread::sleep_for(std::chrono::seconds(2));
            }
        }).detach();
    }
    
    void update_hardware_display(const HardwareStats& stats) {
        // Update CPU
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(cpu_progress), stats.cpu_usage / 100.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(cpu_progress), 
                                 (std::to_string((int)stats.cpu_usage) + "%").c_str());
        
        // Update Memory
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(memory_progress), stats.memory_usage / 100.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(memory_progress), 
                                 (std::to_string((int)stats.memory_usage) + "%").c_str());
        
        // Update Disk
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(disk_progress), stats.disk_usage / 100.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(disk_progress), 
                                 (std::to_string((int)stats.disk_usage) + "%").c_str());
        
        // Update Temperatures
        gtk_label_set_text(GTK_LABEL(cpu_temp_label), 
                          ("CPU: " + std::to_string(stats.cpu_temp) + "°C").c_str());
        gtk_label_set_text(GTK_LABEL(gpu_temp_label), 
                          ("GPU: " + std::to_string(stats.gpu_temp) + "°C").c_str());
        
        // Update GPU
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gpu_progress), stats.gpu_usage / 100.0);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(gpu_progress), 
                                 (std::to_string((int)stats.gpu_usage) + "%").c_str());
        
        // Update Network
        gtk_label_set_text(GTK_LABEL(network_label), 
                          ("↓" + std::to_string((int)stats.network_rx) + " ↑" + 
                           std::to_string((int)stats.network_tx) + " KB/s").c_str());
        
        // Update System Load
        GtkWidget *hw_box = gtk_widget_get_parent(cpu_progress);
        GtkWidget *load_label = GTK_WIDGET(g_object_get_data(G_OBJECT(hw_box), "load_label"));
        if (load_label) {
            gtk_label_set_text(GTK_LABEL(load_label), ("Load: " + stats.system_load).c_str());
        }
        
        // Update Connections
        GtkWidget *conn_label = GTK_WIDGET(g_object_get_data(G_OBJECT(hw_box), "conn_label"));
        if (conn_label) {
            gtk_label_set_text(GTK_LABEL(conn_label), ("Connections: " + std::to_string(stats.active_connections)).c_str());
        }
    }

    void analyze_logs() {
        StructuredLogger::user_action("Log Analysis Started");
        
        if (is_running.load()) {
            update_status("Analysis already in progress...");
            StructuredLogger::warn("log_analysis", "/gui", "Analysis already in progress");
            return;
        }
        
        is_running.store(true);
        gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.1);
        gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Initializing...");
        update_status("🔄 Starting log analysis...");
        
        // Get values safely with security validation
        gchar* level_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_level));
        gchar* unit_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo_unit));
        gchar* since_combo_text = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(entry_since));
        const gchar* tail_text = gtk_entry_get_text(GTK_ENTRY(entry_tail));
        
        std::string level = level_text ? level_text : "ALL";
        std::string unit = unit_text ? unit_text : "All Services";
        std::string since = since_combo_text ? since_combo_text : "";
        std::string tail = tail_text ? tail_text : "100";
        
        // Free allocated strings
        if (level_text) g_free(level_text);
        if (unit_text) g_free(unit_text);
        if (since_combo_text) g_free(since_combo_text);
        
        bool summary = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_summary));
        bool csv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_csv));
        bool watch = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(check_watch));

        // Security validation
        EnhancedSecurity::log_security_event("Log analysis started");
        
        std::string cmd = "journalctl -b -o json -a --no-pager";
        
        if (!EnhancedSecurity::is_safe_command(cmd)) {
            update_status("Security: Command blocked");
            is_running.store(false);
            return;
        }

        // Add filters with security validation
        if (unit != "All Services") {
            std::string safe_unit = SecurityValidator::sanitize_unit_input(unit);
            if (!safe_unit.empty()) {
                cmd += " -u " + safe_unit;
            }
        }
        
        if (!since.empty()) {
            std::string safe_since = SecurityValidator::sanitize_time_input(since);
            if (!safe_since.empty()) {
                cmd += " --since='" + safe_since + "'";
            }
        }
        
        if (!tail.empty() && tail != "0") {
            if (SecurityValidator::is_valid_number(tail)) {
                cmd += " -n " + tail;
            }
        }
        
        if (watch) {
            cmd += " -f";
        }

        StructuredLogger::system("journalctl", "/usr/bin", "Executing: " + cmd);
        
        // Execute in thread
        std::thread([this, cmd, level, summary, csv, watch]() {
            g_idle_add([](gpointer data) -> gboolean {
                ModernArchLogGUI* gui = static_cast<ModernArchLogGUI*>(data);
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gui->progress_bar), 0.3);
                gtk_progress_bar_set_text(GTK_PROGRESS_BAR(gui->progress_bar), "Executing...");
                gui->update_status("⚙️ Executing journalctl command...");
                return G_SOURCE_REMOVE;
            }, this);

            FILE* pipe = popen(cmd.c_str(), "r");
            if (!pipe) {
                StructuredLogger::error("journalctl", "/usr/bin", "Failed to execute command: " + cmd);
                g_idle_add([](gpointer data) -> gboolean {
                    ModernArchLogGUI* gui = static_cast<ModernArchLogGUI*>(data);
                    gui->append_text("[ERROR] [SYSTEM] journalctl (/usr/bin) | Could not execute journalctl command\n");
                    gui->update_status("Error: Command execution failed");
                    gui->is_running.store(false);
                    return G_SOURCE_REMOVE;
                }, this);
                return;
            }

            char buffer[4096];
            std::string output;
            int entry_count = 0;
            int max_entries = watch ? 10000 : 1000;
            
            while (fgets(buffer, sizeof(buffer), pipe) != NULL && (!watch || entry_count < max_entries)) {
                std::string line(buffer);
                
                if (line.find('{') != std::string::npos) {
                    // Process JSON entry with structured logging
                    std::string formatted_line = format_log_entry(line, entry_count);
                    output += formatted_line;
                    entry_count++;
                }
                
                if (output.length() > 2000) {
                    auto output_copy = std::make_shared<std::string>(output);
                    g_idle_add([](gpointer data) -> gboolean {
                        auto pair = static_cast<std::pair<ModernArchLogGUI*, std::shared_ptr<std::string>>*>(data);
                        pair->first->append_text(*(pair->second));
                        delete pair;
                        return G_SOURCE_REMOVE;
                    }, new std::pair<ModernArchLogGUI*, std::shared_ptr<std::string>>(this, output_copy));
                    output.clear();
                }
            }
            pclose(pipe);

            if (!output.empty()) {
                auto output_copy = std::make_shared<std::string>(output);
                g_idle_add([](gpointer data) -> gboolean {
                    auto pair = static_cast<std::pair<ModernArchLogGUI*, std::shared_ptr<std::string>>*>(data);
                    pair->first->append_text(*(pair->second));
                    delete pair;
                    return G_SOURCE_REMOVE;
                }, new std::pair<ModernArchLogGUI*, std::shared_ptr<std::string>>(this, output_copy));
            }

            g_idle_add([](gpointer data) -> gboolean {
                auto pair = static_cast<std::pair<ModernArchLogGUI*, int>*>(data);
                ModernArchLogGUI* gui = pair->first;
                int count = pair->second;
                gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(gui->progress_bar), 1.0);
                gtk_progress_bar_set_text(GTK_PROGRESS_BAR(gui->progress_bar), "Complete");
                if (count == 0) {
                    gui->update_status("⚠️ No logs found - try different filters");
                    StructuredLogger::warn("log_analysis", "/gui", "No logs found with current filters");
                } else {
                    gui->update_status("✅ Analysis completed - " + std::to_string(count) + " entries found");
                    StructuredLogger::info("log_analysis", "/gui", "Analysis completed: " + std::to_string(count) + " entries");
                }
                gui->is_running.store(false);
                delete pair;
                return G_SOURCE_REMOVE;
            }, new std::pair<ModernArchLogGUI*, int>(this, entry_count));
        }).detach();
    }

    void save_output() {
        GtkWidget *dialog = gtk_file_chooser_dialog_new("Save Log Output",
                                                       GTK_WINDOW(window),
                                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                                       "_Cancel", GTK_RESPONSE_CANCEL,
                                                       "_Save", GTK_RESPONSE_ACCEPT,
                                                       NULL);
        
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "archlog_output.txt");
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            
            GtkTextIter start, end;
            gtk_text_buffer_get_bounds(buffer, &start, &end);
            char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
            
            FILE *file = fopen(filename, "w");
            if (file) {
                fputs(text, file);
                fclose(file);
                update_status("Output saved to: " + std::string(filename));
            } else {
                update_status("Error: Could not save file");
            }
            
            g_free(filename);
            g_free(text);
        }
        
        gtk_widget_destroy(dialog);
    }

    void refresh_units() {
        GtkTreeModel *model = gtk_combo_box_get_model(GTK_COMBO_BOX(combo_unit));
        gtk_list_store_clear(GTK_LIST_STORE(model));
        
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_unit), "All Services");
        
        const char* common_units[] = {
            "kernel", "systemd", "NetworkManager.service", "sshd.service",
            "bluetooth.service", "user@1000.service", "dbus.service"
        };
        
        for (const char* unit : common_units) {
            gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo_unit), unit);
        }
        
        gtk_combo_box_set_active(GTK_COMBO_BOX(combo_unit), 0);
        update_status("🚀 Ready - ArchVault System Monitor");
    }

    void show_system_info() {
        std::string info = ArchFeatures::get_system_info();
        info += ArchFeatures::get_boot_time();
        
        auto failed = ArchFeatures::get_failed_services();
        if (!failed.empty()) {
            info += "\n=== FAILED SERVICES ===\n";
            for (const auto& service : failed) {
                info += "❌ " + service + "\n";
            }
        } else {
            info += "\n✅ All services running normally\n";
        }
        
        append_text("\n" + info + "\n");
        update_status("System information displayed");
    }

    void show_pacman_logs() {
        auto logs = ArchFeatures::get_pacman_logs();
        
        append_text("\n=== RECENT PACMAN ACTIVITY ===\n");
        if (logs.empty()) {
            append_text("No recent package activity found.\n");
        } else {
            for (const auto& log : logs) {
                append_text(log);
            }
        }
        append_text("\n");
        update_status("Pacman logs displayed");
    }
    
    void execute_quick_action(int index) {
        auto quick_filters = QuickActions::get_quick_filters();
        if (index >= 0 && index < static_cast<int>(quick_filters.size())) {
            const auto& filter = quick_filters[index];
            append_text("\n=== " + filter.first + " ===\n");
            update_status("Executing: " + filter.first);
            
            FILE* pipe = popen(filter.second.c_str(), "r");
            if (pipe) {
                char buffer[1024];
                bool has_output = false;
                while (fgets(buffer, sizeof(buffer), pipe)) {
                    append_text(std::string(buffer));
                    has_output = true;
                }
                int exit_code = pclose(pipe);
                
                if (!has_output) {
                    append_text("No results found for this filter.\n");
                }
                
                if (exit_code != 0) {
                    append_text("Warning: Command exited with code " + std::to_string(exit_code) + "\n");
                }
            } else {
                append_text("Error: Could not execute command '" + filter.second + "'\n");
                append_text("This may be due to missing permissions or unavailable tools.\n");
                update_status("Error executing: " + filter.first);
            }
            append_text("\n");
            update_status("Quick action completed: " + filter.first);
        } else {
            append_text("Error: Invalid quick action index\n");
            update_status("Error: Invalid action");
        }
    }
    
    void show_top_processes() {
        append_text("\n=== TOP CPU PROCESSES ===\n");
        update_status("Loading process information...");
        
        FILE* pipe = popen("ps aux --sort=-%cpu | head -10", "r");
        if (pipe) {
            char buffer[256];
            bool has_output = false;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                append_text(std::string(buffer));
                has_output = true;
            }
            pclose(pipe);
            
            if (!has_output) {
                append_text("No process information available.\n");
            }
        } else {
            append_text("Error: Could not retrieve process information.\n");
            append_text("The 'ps' command may not be available.\n");
        }
        append_text("\n");
        update_status("Process information displayed");
    }
    
    void show_network_status() {
        append_text("\n=== NETWORK STATUS ===\n");
        update_status("Loading network information...");
        
        FILE* pipe = popen("ss -tuln | head -10", "r");
        if (pipe) {
            char buffer[256];
            bool has_output = false;
            while (fgets(buffer, sizeof(buffer), pipe)) {
                append_text(std::string(buffer));
                has_output = true;
            }
            pclose(pipe);
            
            if (!has_output) {
                append_text("No network connections found.\n");
            }
        } else {
            append_text("Error: Could not retrieve network status.\n");
            append_text("Trying alternative method...\n");
            
            FILE* alt_pipe = popen("netstat -tuln 2>/dev/null | head -10", "r");
            if (alt_pipe) {
                char buffer[256];
                while (fgets(buffer, sizeof(buffer), alt_pipe)) {
                    append_text(std::string(buffer));
                }
                pclose(alt_pipe);
            } else {
                append_text("Network tools not available.\n");
            }
        }
        append_text("\n");
        update_status("Network status displayed");
    }
    
    void show_services_status() {
        append_text("\n=== SYSTEMD SERVICES STATUS ===\n");
        update_status("Loading services information...");
        
        // Failed services
        FILE* pipe = popen("systemctl --failed --no-legend --no-pager 2>/dev/null", "r");
        if (pipe) {
            char buffer[256];
            bool has_failed = false;
            append_text("Failed Services:\n");
            while (fgets(buffer, sizeof(buffer), pipe)) {
                append_text("❌ " + std::string(buffer));
                has_failed = true;
            }
            pclose(pipe);
            if (!has_failed) {
                append_text("✅ No failed services\n");
            }
        }
        
        // Active services count
        pipe = popen("systemctl list-units --type=service --state=active --no-legend --no-pager | wc -l", "r");
        if (pipe) {
            char buffer[64];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                append_text("\nActive Services: " + std::string(buffer));
            }
            pclose(pipe);
        }
        
        append_text("\n");
        update_status("Services status displayed");
    }
    
    void show_disk_info() {
        append_text("\n=== DISK INFORMATION ===\n");
        update_status("Loading disk information...");
        
        FILE* pipe = popen("df -h | grep -E '^/dev'", "r");
        if (pipe) {
            char buffer[256];
            append_text("Filesystem Usage:\n");
            while (fgets(buffer, sizeof(buffer), pipe)) {
                append_text(std::string(buffer));
            }
            pclose(pipe);
        }
        
        // Disk I/O stats
        pipe = popen("iostat -d 1 1 2>/dev/null | tail -n +4 | head -5", "r");
        if (pipe) {
            char buffer[256];
            append_text("\nDisk I/O Statistics:\n");
            while (fgets(buffer, sizeof(buffer), pipe)) {
                append_text(std::string(buffer));
            }
            pclose(pipe);
        }
        
        append_text("\n");
        update_status("Disk information displayed");
    }
    
    void export_analysis() {
        GtkWidget *dialog = gtk_file_chooser_dialog_new("Export Analysis",
                                                       GTK_WINDOW(window),
                                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                                       "_Cancel", GTK_RESPONSE_CANCEL,
                                                       "_Export", GTK_RESPONSE_ACCEPT,
                                                       NULL);
        
        // Add file filters
        GtkFileFilter *filter_txt = gtk_file_filter_new();
        gtk_file_filter_set_name(filter_txt, "Text Files (*.txt)");
        gtk_file_filter_add_pattern(filter_txt, "*.txt");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_txt);
        
        GtkFileFilter *filter_csv = gtk_file_filter_new();
        gtk_file_filter_set_name(filter_csv, "CSV Files (*.csv)");
        gtk_file_filter_add_pattern(filter_csv, "*.csv");
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter_csv);
        
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "archlog_analysis.txt");
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            
            GtkTextIter start, end;
            gtk_text_buffer_get_bounds(buffer, &start, &end);
            char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
            
            FILE *file = fopen(filename, "w");
            if (file) {
                fprintf(file, "# ArchLog Analysis Export\n");
                fprintf(file, "# Generated: %s\n", get_current_time().c_str());
                fprintf(file, "# System: %s\n\n", get_system_info_brief().c_str());
                fputs(text, file);
                fclose(file);
                update_status("📤 Analysis exported to: " + std::string(filename));
            } else {
                update_status("❌ Error: Could not export file");
            }
            
            g_free(filename);
            g_free(text);
        }
        
        gtk_widget_destroy(dialog);
    }
    
    void stop_analysis() {
        if (is_running.load()) {
            is_running.store(false);
            monitor_running.store(false);
            gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress_bar), 0.0);
            gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress_bar), "Stopped");
            update_status("⏹️ Analysis stopped by user");
        } else {
            update_status("ℹ️ No analysis running");
        }
    }
    
    std::string get_current_time() {
        time_t now = time(0);
        struct tm* tm_info = localtime(&now);
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        return std::string(time_str);
    }
    
    std::string get_system_info_brief() {
        std::string info = "Arch Linux";
        FILE* pipe = popen("uname -r 2>/dev/null", "r");
        if (pipe) {
            char buffer[128];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                std::string kernel(buffer);
                kernel.erase(kernel.find_last_not_of(" \n\r\t") + 1);
                info += " (" + kernel + ")";
            }
            pclose(pipe);
        }
        return info;
    }
    
    void run_security_scan() {
        StructuredLogger::security("security_scan", "/gui", "Security scan initiated");
        append_text("\n=== SECURITY SCAN ===\n");
        update_status("Running security scan...");
        
        // Check failed logins
        append_text("[SECURITY] auth_scan (/var/log) | Checking failed login attempts...\n");
        FILE* pipe = popen("journalctl _COMM=sshd | grep 'Failed password' | tail -10", "r");
        if (pipe) {
            char buffer[512];
            int count = 0;
            while (fgets(buffer, sizeof(buffer), pipe) && count < 10) {
                append_text("[WARN] [SECURITY] sshd (/var/log/auth) | " + std::string(buffer));
                count++;
            }
            pclose(pipe);
            if (count == 0) {
                append_text("[INFO] [SECURITY] auth_scan (/var/log) | No failed login attempts found\n");
            }
        }
        
        // Check sudo usage
        append_text("\n[SECURITY] sudo_scan (/var/log) | Checking sudo usage...\n");
        pipe = popen("journalctl _COMM=sudo | tail -5", "r");
        if (pipe) {
            char buffer[512];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                append_text("[INFO] [SECURITY] sudo (/var/log/auth) | " + std::string(buffer));
            }
            pclose(pipe);
        }
        
        StructuredLogger::security("security_scan", "/gui", "Security scan completed");
        update_status("Security scan completed");
    }
    
    void show_performance_analysis() {
        StructuredLogger::performance("perf_analysis", "/gui", "Performance analysis started");
        append_text("\n=== PERFORMANCE ANALYSIS ===\n");
        update_status("Analyzing system performance...");
        
        auto stats = HardwareMonitor::get_current_stats();
        
        append_text("[INFO] [PERFORMANCE] cpu_monitor (/proc/stat) | CPU Usage: " + 
                   std::to_string((int)stats.cpu_usage) + "%\n");
        append_text("[INFO] [PERFORMANCE] mem_monitor (/proc/meminfo) | Memory Usage: " + 
                   std::to_string((int)stats.memory_usage) + "%\n");
        append_text("[INFO] [PERFORMANCE] disk_monitor (/proc/diskstats) | Disk Usage: " + 
                   std::to_string((int)stats.disk_usage) + "%\n");
        append_text("[INFO] [PERFORMANCE] load_monitor (/proc/loadavg) | System Load: " + 
                   stats.system_load + "\n");
        
        // Top processes
        append_text("\n[INFO] [PERFORMANCE] process_monitor (/proc) | Top CPU processes:\n");
        FILE* pipe = popen("ps aux --sort=-%cpu | head -5 | tail -4", "r");
        if (pipe) {
            char buffer[256];
            while (fgets(buffer, sizeof(buffer), pipe)) {
                append_text("[INFO] [PERFORMANCE] ps (/proc) | " + std::string(buffer));
            }
            pclose(pipe);
        }
        
        std::map<std::string, std::string> metrics = {
            {"cpu_usage", std::to_string(stats.cpu_usage)},
            {"memory_usage", std::to_string(stats.memory_usage)},
            {"disk_usage", std::to_string(stats.disk_usage)}
        };
        StructuredLogger::performance("perf_analysis", "/gui", "Performance analysis completed", metrics);
        update_status("Performance analysis completed");
    }
    
    void export_structured_logs() {
        StructuredLogger::user_action("Export Structured Logs");
        
        GtkWidget *dialog = gtk_file_chooser_dialog_new("Export Structured Logs",
                                                       GTK_WINDOW(window),
                                                       GTK_FILE_CHOOSER_ACTION_SAVE,
                                                       "_Cancel", GTK_RESPONSE_CANCEL,
                                                       "_Export", GTK_RESPONSE_ACCEPT,
                                                       NULL);
        
        gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dialog), "archlog_structured.log");
        
        if (gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
            char *filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            
            FILE *file = fopen(filename, "w");
            if (file) {
                fprintf(file, "# ArchLog Structured Export\n");
                fprintf(file, "# Format: [ID] [TIME] [LEVEL] [CATEGORY] log_name (directory) | message\n\n");
                
                // Export current buffer content
                GtkTextIter start, end;
                gtk_text_buffer_get_bounds(buffer, &start, &end);
                char *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
                fputs(text, file);
                g_free(text);
                
                fclose(file);
                update_status("📋 Structured logs exported to: " + std::string(filename));
                StructuredLogger::info("export", "/gui", "Logs exported to " + std::string(filename));
            } else {
                update_status("❌ Error: Could not export file");
                StructuredLogger::error("export", "/gui", "Failed to export to " + std::string(filename));
            }
            
            g_free(filename);
        }
        
        gtk_widget_destroy(dialog);
    }

    void append_text(const std::string& text) {
        std::lock_guard<std::mutex> lock(buffer_mutex);
        GtkTextIter end;
        gtk_text_buffer_get_end_iter(buffer, &end);
        gtk_text_buffer_insert(buffer, &end, text.c_str(), -1);
        
        gtk_text_buffer_get_end_iter(buffer, &end);
        GtkTextMark *mark = gtk_text_buffer_create_mark(buffer, NULL, &end, FALSE);
        gtk_text_view_scroll_mark_onscreen(GTK_TEXT_VIEW(text_view), mark);
        gtk_text_buffer_delete_mark(buffer, mark);
    }
    
    void update_status(const std::string& message) {
        gtk_label_set_text(GTK_LABEL(status_label), message.c_str());
    }
    
    std::string format_log_entry(const std::string& json_line, int entry_num) {
        // Extract key fields from JSON and format as structured log
        std::string timestamp = extract_json_field(json_line, "__REALTIME_TIMESTAMP");
        std::string message = extract_json_field(json_line, "MESSAGE");
        std::string unit = extract_json_field(json_line, "_SYSTEMD_UNIT");
        std::string priority = extract_json_field(json_line, "PRIORITY");
        std::string comm = extract_json_field(json_line, "_COMM");
        
        if (unit.empty()) unit = comm.empty() ? "system" : comm;
        if (message.empty()) message = "No message";
        
        std::string level = priority_to_level_name(priority);
        std::string formatted_time = format_timestamp(timestamp);
        
        std::stringstream formatted;
        formatted << "[" << std::setfill('0') << std::setw(4) << entry_num << "] "
                 << "[" << formatted_time << "] "
                 << "[" << level << "] "
                 << "[SYSTEM] "
                 << unit << " (/var/log/journal) | "
                 << message << "\n";
        
        return formatted.str();
    }
    
    std::string extract_json_field(const std::string& json, const std::string& field) {
        std::string search = "\"" + field + "\"";
        size_t pos = json.find(search);
        if (pos == std::string::npos) return "";
        
        pos = json.find(':', pos);
        if (pos == std::string::npos) return "";
        
        pos = json.find('"', pos);
        if (pos == std::string::npos) return "";
        pos++;
        
        size_t end = json.find('"', pos);
        if (end == std::string::npos) return "";
        
        return json.substr(pos, end - pos);
    }
    
    std::string priority_to_level_name(const std::string& priority) {
        if (priority.empty()) return "INFO";
        int p = std::stoi(priority);
        switch (p) {
            case 0: return "EMERG";
            case 1: return "ALERT";
            case 2: return "CRIT";
            case 3: return "ERROR";
            case 4: return "WARN";
            case 5: return "NOTICE";
            case 6: return "INFO";
            case 7: return "DEBUG";
            default: return "INFO";
        }
    }
    
    std::string format_timestamp(const std::string& us_timestamp) {
        if (us_timestamp.empty()) return get_current_time();
        try {
            long long us = std::stoll(us_timestamp);
            time_t seconds = us / 1000000;
            struct tm* tm_info = localtime(&seconds);
            char buffer[32];
            strftime(buffer, sizeof(buffer), "%H:%M:%S", tm_info);
            return std::string(buffer);
        } catch (...) {
            return get_current_time();
        }
    }
};

int main(int, char**) {
    StructuredLogger::initialize();
    StructuredLogger::system("archlog_gui", "/usr/bin", "ArchLog GUI starting");
    
    try {
        ModernArchLogGUI gui;
        if (gui.is_initialized()) {
            gui.run();
        } else {
            StructuredLogger::warn("archlog_gui", "/usr/bin", "GUI not initialized - headless mode");
            return 0;
        }
        return 0;
    } catch (const std::exception& e) {
        StructuredLogger::error("archlog_gui", "/usr/bin", std::string("GUI Error: ") + e.what());
        return 1;
    }
}