# Contributing to ArchVault

Thank you for your interest in contributing to ArchVault! This document provides guidelines for contributing to the project.

## 🚀 Getting Started

1. Fork the repository
2. Clone your fork: `git clone https://github.com/yourusername/archvault.git`
3. Create a new branch: `git checkout -b feature/your-feature-name`

## 🛠️ Development Setup

### Prerequisites
```bash
sudo pacman -S gtk3 pkg-config gcc make cmake base-devel
```

### Building
```bash
./build.sh  # Build both CLI and GUI
```

## 📝 Code Style

- Use C++17 features
- Follow existing naming conventions
- Add comments for complex logic
- Keep functions focused and small
- Use proper error handling

## 🧪 Testing

Before submitting:
```bash
# Test CLI
./archlog --help
./archlog --summary --tail=10

# Test GUI
./archlog-gui
```

## 📋 Pull Request Process

1. Update documentation if needed
2. Test your changes thoroughly
3. Follow the existing code style
4. Write clear commit messages
5. Submit a pull request with a clear description

## 🐛 Bug Reports

Include:
- OS version and environment
- Steps to reproduce
- Expected vs actual behavior
- Error messages or logs

## 💡 Feature Requests

- Describe the feature clearly
- Explain the use case
- Consider implementation complexity

## 📄 License

By contributing, you agree that your contributions will be licensed under the MIT License.