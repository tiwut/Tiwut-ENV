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
    sudo apt-get install -y cmake g++ pkg-config libwayland-dev
elif [[ "$OS" == "fedora" || "$OS" == "bazzite" || "$OS_LIKE" == *"fedora"* ]]; then
    echo "Installing dependencies for Fedora/Bazzite..."
    sudo dnf install -y cmake gcc-c++ pkgconf-pkg-config wayland-devel
elif [[ "$OS" == "arch" || "$OS_LIKE" == *"arch"* ]]; then
    echo "Installing dependencies for Arch Linux..."
    sudo pacman -Syu --noconfirm cmake gcc pkgconf wayland
else
    echo "Unsupported OS for auto-install. Please install cmake, g++, pkg-config, and wayland-server headers manually."
fi

echo "Dependencies installed successfully!"
echo "Building Tiwut-ENV core compositor..."

mkdir -p build
cd build
rm -rf *
cmake ..
make -j$(nproc)

echo "================================================="
echo "Build complete! You can run it with: ./build/tiwut-env"
echo "================================================="
