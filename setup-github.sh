#!/bin/bash

# ArchVault GitHub Setup Script
echo "🚀 Setting up ArchVault for GitHub..."

# Initialize git repository if not already done
if [ ! -d ".git" ]; then
    echo "📁 Initializing Git repository..."
    git init
    git branch -M main
fi

# Add all files
echo "📝 Adding files to Git..."
git add .

# Create initial commit
echo "💾 Creating initial commit..."
git commit -m "🎉 Initial commit: ArchVault - Advanced System Log Analyzer

✨ Features:
- CLI and GUI interfaces
- Real-time system monitoring
- Hardware monitoring (CPU, Memory, Disk, GPU)
- Security scanning and log analysis
- Modern dark-themed GUI
- Export capabilities
- Structured logging system

🛠️ Built with C++17, GTK3, and modern design principles"

echo ""
echo "🎯 Next steps to push to GitHub:"
echo ""
echo "1. Create a new repository on GitHub named 'archvault'"
echo "2. Run these commands:"
echo ""
echo "   git remote add origin https://github.com/YOUR_USERNAME/archvault.git"
echo "   git push -u origin main"
echo ""
echo "📋 Repository is ready for GitHub!"
echo ""
echo "🔗 Suggested repository name: archvault"
echo "📝 Suggested description: Advanced System Log Analyzer for Arch Linux with CLI and GUI interfaces"
echo "🏷️ Suggested topics: arch-linux, system-monitoring, log-analyzer, cpp, gtk3, security"