# ArchVault - System Log Analyzer

Advanced system log analyzer for Arch Linux with CLI and GUI interfaces.

## Features

- **CLI Interface**: Fast command-line log analysis
- **GUI Interface**: Modern dark-themed graphical interface
- **Real-time Monitoring**: Live system stats and log streaming
- **Hardware Monitor**: CPU, Memory, Disk, GPU usage
- **Security Analysis**: Failed logins, sudo usage tracking

## Build

```bash
# Install dependencies
sudo pacman -S gtk3 pkg-config gcc make

# Build
./build.sh

# Run
./archlog      # CLI
./archlog-gui  # GUI
```

## Usage

```bash
# CLI examples
./archlog --summary
./archlog -m ERROR --tail=50
./archlog --csv --no-filter

# GUI
./archlog-gui
```

## License

MIT License - see LICENSE file.