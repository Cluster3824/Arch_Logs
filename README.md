# 🚀 ArchVault - Advanced System Log Analyzer

**ArchVault** is a powerful, modern system log analyzer designed specifically for Arch Linux systems. It provides both CLI and GUI interfaces for comprehensive system monitoring, log analysis, and security auditing.

## ✨ Features

### 🖥️ Dual Interface
- **CLI Mode**: Fast command-line interface for scripts and automation
- **GUI Mode**: Modern dark-themed graphical interface with real-time monitoring

### 📊 Advanced Analytics
- **Real-time Log Analysis**: Live monitoring of system logs
- **Hardware Monitoring**: CPU, Memory, Disk, GPU usage and temperatures
- **Security Scanning**: Failed login detection, sudo usage tracking
- **Performance Analysis**: System load, network statistics, process monitoring

### 🔒 Security Features
- **Enhanced Security Validation**: Input sanitization and command validation
- **Structured Logging**: Professional logging with categorization
- **Permission Management**: Safe execution with proper privilege handling

### 🎨 Modern UI
- **Dark Theme**: Professional dark interface optimized for long usage
- **Real-time Updates**: Live hardware monitoring and log streaming
- **Export Capabilities**: Save analysis results in multiple formats

## 🛠️ Installation

### Prerequisites
```bash
# Install dependencies
sudo pacman -S gtk3 pkg-config gcc make cmake

# For development
sudo pacman -S base-devel git
```

### Build from Source
```bash
# Clone the repository
git clone https://github.com/yourusername/archvault.git
cd archvault

# Build both CLI and GUI versions
./build.sh

# Or build individually
make                    # CLI version
make -f Makefile.gui   # GUI version
```

### Installation
```bash
# Install system-wide
sudo make install

# Or run locally
./archlog      # CLI version
./archlog-gui  # GUI version
```

## 🚀 Usage

### CLI Interface
```bash
# Basic usage
./archlog --summary

# Filter by severity
./archlog -m ERROR --tail=50

# Export to CSV
./archlog --csv --summary > system_analysis.csv

# Real-time monitoring
./archlog --follow --no-filter
```

### GUI Interface
```bash
# Launch GUI
./archlog-gui
```

**GUI Features:**
- 🎛️ **Control Panel**: Easy filtering and configuration
- 📈 **Hardware Monitor**: Real-time system statistics
- 🔍 **Quick Actions**: Pre-configured analysis filters
- 💾 **Export Tools**: Save and export analysis results

## 📋 Command Line Options

| Option | Description |
|--------|-------------|
| `--summary` | Show analysis summary with all entries |
| `-m LEVEL` | Set minimum severity level |
| `--no-filter` | Show all messages including DEBUG |
| `--tail=N` | Show only last N entries |
| `--csv` | Output in CSV format |
| `--help` | Show help information |

## 🔧 Configuration

### Severity Levels
- **EMERG(0)**: System is unusable
- **ALERT(1)**: Action must be taken immediately  
- **CRIT(2)**: Critical conditions
- **ERROR(3)**: Error conditions
- **WARNING(4)**: Warning conditions
- **NOTICE(5)**: Normal but significant (default)
- **INFO(6)**: Informational
- **DEBUG(7)**: Debug-level messages

## 🏗️ Architecture

```
archvault/
├── src/
│   ├── main.cpp              # CLI application
│   ├── modern_gui.cpp        # GUI application
│   ├── security.h            # Security validation
│   ├── structured_logger.h   # Logging system
│   ├── hardware_monitor.h    # Hardware monitoring
│   ├── arch_features.h       # Arch-specific features
│   ├── enhanced_security.h   # Advanced security
│   └── quick_actions.h       # Predefined filters
├── build.sh                  # Build script
├── Makefile                  # CLI build
├── Makefile.gui             # GUI build
└── CMakeLists.txt           # CMake configuration
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Built for the Arch Linux community
- Inspired by modern system monitoring tools
- Uses GTK3 for the graphical interface

## 📞 Support

- 🐛 **Issues**: [GitHub Issues](https://github.com/yourusername/archvault/issues)
- 💬 **Discussions**: [GitHub Discussions](https://github.com/yourusername/archvault/discussions)
- 📧 **Email**: your.email@example.com

---

**Made with ❤️ for Arch Linux users**