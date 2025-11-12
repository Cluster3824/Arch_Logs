#!/bin/bash

echo "🚀 Setting up ArchVault for GitHub..."

# Git configuration
git config user.name "Cluster3824"
git config user.email "rohith3824r1@gmail.com"

# Initialize if needed
if [ ! -d ".git" ]; then
    git init
    git branch -M main
fi

# Add and commit
git add .
git commit -m "🎨 ArchVault: Arch Linux aesthetic theme

✨ Features:
- Arch Linux inspired color scheme
- Catppuccin-style dark theme  
- CLI and GUI interfaces
- Real-time system monitoring
- Hardware stats display
- Security log analysis

🎨 Theme: Deep purple/blue with cyan accents
🖥️ Built for Arch Linux enthusiasts"

echo ""
echo "📋 Next steps:"
echo "1. Create repository: https://github.com/new"
echo "2. Repository name: archvault"
echo "3. Description: Advanced System Log Analyzer with Arch Linux aesthetic"
echo ""
echo "4. Run these commands:"
echo "   git remote add origin https://github.com/Cluster3824/archvault.git"
echo "   git push -u origin main"
echo ""
echo "✅ Ready for GitHub!"