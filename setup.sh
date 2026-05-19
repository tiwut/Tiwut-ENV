#!/bin/bash
set -e

echo "================================================="
echo "  Tiwut-ENV Automated Setup & Build Script"
echo "================================================="

if [ -f /etc/os-release ]; then
    . /etc/os-release
    OS=$ID
    OS_LIKE=$ID_LIKE
else
    echo "Cannot detect OS. Please install dependencies manually."
    exit 1
fi

echo "Detecting OS... Found: $OS"

if [[ "$OS" == "ubuntu" || "$OS" == "debian" || "$OS_LIKE" == *"ubuntu"* || "$OS_LIKE" == *"debian"* ]]; then
    echo "Installing dependencies for Debian/Ubuntu..."
    sudo apt-get update -y
    
    WLROOTS_PKG="libwlroots-dev"
    if apt-cache search libwlroots-0.18-dev | grep -q libwlroots-0.18-dev; then
        WLROOTS_PKG="libwlroots-0.18-dev"
    elif apt-cache search libwlroots-0.17-dev | grep -q libwlroots-0.17-dev; then
        WLROOTS_PKG="libwlroots-0.17-dev"
    fi
    echo "Using wlroots package: $WLROOTS_PKG"
    
    sudo apt-get install -y cmake g++ pkg-config libwayland-dev libxkbcommon-dev wayland-protocols chromium $WLROOTS_PKG || {
        echo "Error: Failed to install Debian dependencies."
        exit 1
    }
    
elif [[ "$OS" == "fedora" || "$OS" == "bazzite" || "$OS_LIKE" == *"fedora"* ]]; then
    echo "Installing dependencies for Fedora/Bazzite..."
    sudo dnf install -y cmake gcc-c++ pkgconf-pkg-config wayland-devel wlroots-devel libxkbcommon-devel wayland-protocols-devel chromium || {
        echo "Error: dnf install failed."
        if [[ "$OS" == "bazzite" || -f /run/ostree-booted ]]; then
            echo "You are on an atomic system (Bazzite/Silverblue). Please run this script inside a Distrobox container!"
        fi
        exit 1
    }
    
elif [[ "$OS" == "arch" || "$OS_LIKE" == *"arch"* ]]; then
    echo "Installing dependencies for Arch Linux..."
    sudo pacman -Syu --noconfirm cmake gcc pkgconf wayland wlroots libxkbcommon wayland-protocols chromium || {
        echo "Error: Failed to install Arch dependencies."
        exit 1
    }
else
    echo "Unsupported OS for auto-install. Please install cmake, g++, pkg-config, wayland-server, wlroots, libxkbcommon, and wayland-protocols manually."
    exit 1
fi

echo "Dependencies installed successfully!"
echo "Building Tiwut-ENV core compositor..."

mkdir -p build
cd build
rm -rf *
cmake .. || { echo "CMake configuration failed!"; exit 1; }
make -j$(nproc) || { echo "Compilation failed!"; exit 1; }

echo "================================================="
echo "Build complete! You can run it with: ./build/tiwut-env"
echo "================================================="
