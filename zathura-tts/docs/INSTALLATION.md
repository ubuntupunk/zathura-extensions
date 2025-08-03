# Installation Guide

This guide provides detailed installation instructions for the Zathura TTS plugin across different Linux distributions.

## System Requirements

### Minimum Requirements
- **Operating System**: Linux (any modern distribution)
- **Zathura**: Version 0.4.0 or later
- **GLib**: Version 2.50 or later
- **GTK+ 3**: Version 3.22 or later
- **Python**: Version 3.8 or later (for Piper-TTS)

### Recommended Requirements
- **Zathura**: Version 0.5.0 or later
- **girara-gtk3**: Version 0.4.0 or later
- **At least 2GB RAM** (for neural TTS voices)
- **Audio system**: PulseAudio or ALSA

## Distribution-Specific Installation

### Ubuntu/Debian

#### Install System Dependencies
```bash
# Update package list
sudo apt update

# Install Zathura and development libraries
sudo apt install zathura zathura-dev libgirara-dev libglib2.0-dev libgtk-3-dev

# Install build tools
sudo apt install meson ninja-build gcc pkg-config

# Install TTS engines
sudo apt install speech-dispatcher libspeechd-dev espeak-ng

# Install Python and pip for Piper-TTS
sudo apt install python3 python3-pip
```

#### Install Piper-TTS
```bash
# Install Piper-TTS via pip
pip3 install piper

# Or install system-wide
sudo pip3 install piper
```

### Fedora/RHEL/CentOS

#### Install System Dependencies
```bash
# Install Zathura and development libraries
sudo dnf install zathura zathura-devel girara-devel glib2-devel gtk3-devel

# Install build tools
sudo dnf install meson ninja-build gcc pkgconfig

# Install TTS engines
sudo dnf install speech-dispatcher speech-dispatcher-devel espeak-ng

# Install Python and pip
sudo dnf install python3 python3-pip
```

#### Install Piper-TTS
```bash
# Install Piper-TTS
pip3 install --user piper
```

### Arch Linux

#### Install System Dependencies
```bash
# Install Zathura and development libraries
sudo pacman -S zathura girara glib2 gtk3

# Install build tools
sudo pacman -S meson ninja gcc pkgconf

# Install TTS engines
sudo pacman -S speech-dispatcher espeak-ng

# Install Python and pip
sudo pacman -S python python-pip
```

#### Install Piper-TTS
```bash
# Install from AUR (if available)
yay -S piper-tts

# Or install via pip
pip install --user piper
```

### openSUSE

#### Install System Dependencies
```bash
# Install Zathura and development libraries
sudo zypper install zathura girara-devel glib2-devel gtk3-devel

# Install build tools
sudo zypper install meson ninja gcc pkgconfig

# Install TTS engines
sudo zypper install speech-dispatcher speech-dispatcher-devel espeak-ng

# Install Python and pip
sudo zypper install python3 python3-pip
```

#### Install Piper-TTS
```bash
# Install Piper-TTS
pip3 install --user piper
```

## Building the Plugin

### Download Source Code
```bash
# Clone the repository
git clone https://github.com/zathura-pdf/zathura-tts.git
cd zathura-tts

# Or download and extract release tarball
wget https://github.com/zathura-pdf/zathura-tts/archive/v1.0.0.tar.gz
tar -xzf v1.0.0.tar.gz
cd zathura-tts-1.0.0
```

### Configure Build
```bash
# Set up build directory
meson setup builddir

# Configure installation prefix (optional)
meson setup builddir --prefix=/usr/local

# Configure for user installation
meson setup builddir --prefix=$HOME/.local
```

### Compile
```bash
# Compile the plugin
meson compile -C builddir

# Check for compilation errors
echo $?  # Should output 0 for success
```

### Install
```bash
# System-wide installation (requires sudo)
sudo meson install -C builddir

# User installation (no sudo required)
meson install -C builddir --destdir=$HOME/.local

# Install to custom location
DESTDIR=/opt/zathura-tts meson install -C builddir
```

### Verify Installation
```bash
# Check if plugin is recognized by Zathura
zathura --list-plugins | grep tts

# Expected output: tts (version x.x.x)
```

## TTS Engine Setup

### Piper-TTS Setup

#### Download Voice Models
```bash
# Create voice directory
mkdir -p ~/.local/share/zathura-tts/voices

# Download English voices
cd ~/.local/share/zathura-tts/voices

# High-quality voice (recommended)
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx.json

# Alternative voices
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/amy/medium/en_US-amy-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/amy/medium/en_US-amy-medium.onnx.json
```

#### Test Piper Installation
```bash
# Test Piper with downloaded voice
echo "Hello, this is a test of Piper TTS" | piper --model ~/.local/share/zathura-tts/voices/en_US-lessac-medium.onnx --output_file test.wav

# Play the generated audio
aplay test.wav  # or paplay test.wav for PulseAudio
```

### Speech Dispatcher Setup

#### Configure Speech Dispatcher
```bash
# Start Speech Dispatcher service
systemctl --user start speech-dispatcher

# Enable automatic startup
systemctl --user enable speech-dispatcher

# Test Speech Dispatcher
spd-say "Hello, this is a test of Speech Dispatcher"
```

#### Configure Default Voice
```bash
# List available voices
spd-say -L

# Test specific voice
spd-say -o espeak-ng -v en "Testing espeak-ng voice"

# Configure default voice in ~/.speechd/conf/speechd.conf
mkdir -p ~/.speechd/conf
echo "DefaultVoiceType espeak-ng" >> ~/.speechd/conf/speechd.conf
```

### espeak-ng Setup

#### Test espeak-ng
```bash
# Basic test
espeak-ng "Hello, this is a test of espeak-ng"

# Test with different voice
espeak-ng -v en-us "Testing American English voice"

# List available voices
espeak-ng --voices
```

## Post-Installation Configuration

### Create Configuration Directory
```bash
# Create Zathura config directory if it doesn't exist
mkdir -p ~/.config/zathura
```

### Basic Configuration
```bash
# Add basic TTS configuration to zathurarc
cat >> ~/.config/zathura/zathurarc << 'EOF'
# TTS Plugin Configuration
set tts_engine piper
set tts_speed 1.0
set tts_volume 80
set tts_highlight_text true
set tts_highlight_color "#3498db"
EOF
```

### Test Installation
```bash
# Open a PDF with Zathura
zathura sample.pdf

# Test TTS functionality
# Press Ctrl+T to start reading
# Press Ctrl+Space to pause/resume
# Press Ctrl+Shift+T to open settings
```

## Troubleshooting Installation

### Common Build Issues

#### Missing Dependencies
```bash
# Error: "Package 'zathura' not found"
# Solution: Install zathura development package
sudo apt install zathura-dev  # Ubuntu/Debian
sudo dnf install zathura-devel  # Fedora
sudo pacman -S zathura  # Arch Linux
```

#### Meson Configuration Fails
```bash
# Error: "Meson version is too old"
# Solution: Install newer Meson version
pip3 install --user meson

# Add to PATH if needed
echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc
source ~/.bashrc
```

#### Compilation Errors
```bash
# Error: "girara/types.h: No such file or directory"
# Solution: Install girara development package
sudo apt install libgirara-dev  # Ubuntu/Debian
sudo dnf install girara-devel   # Fedora
sudo pacman -S girara           # Arch Linux
```

### Plugin Loading Issues

#### Plugin Not Found
```bash
# Check plugin installation location
find /usr -name "*tts*" -type f 2>/dev/null
find ~/.local -name "*tts*" -type f 2>/dev/null

# Check Zathura plugin directory
zathura --print-plugin-dir
```

#### Permission Issues
```bash
# Fix plugin file permissions
chmod 755 ~/.local/lib/zathura/libtts.so

# Fix configuration directory permissions
chmod 755 ~/.config/zathura
chmod 644 ~/.config/zathura/zathurarc
```

### TTS Engine Issues

#### Piper Not Found
```bash
# Check Piper installation
which piper
piper --version

# Install if missing
pip3 install --user piper

# Add to PATH if needed
echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc
```

#### Speech Dispatcher Not Working
```bash
# Check service status
systemctl --user status speech-dispatcher

# Restart service
systemctl --user restart speech-dispatcher

# Check configuration
speechd-conf
```

#### Audio System Issues
```bash
# Check audio system
pulseaudio --check -v

# Restart PulseAudio
pulseaudio -k && pulseaudio --start

# Test audio output
speaker-test -t wav -c 2
```

## Uninstallation

### Remove Plugin
```bash
# Remove installed plugin files
sudo rm -f /usr/local/lib/zathura/libtts.so
rm -f ~/.local/lib/zathura/libtts.so

# Remove configuration
rm -f ~/.config/zathura/tts.conf
```

### Remove TTS Engines (Optional)
```bash
# Remove Piper-TTS
pip3 uninstall piper

# Remove Speech Dispatcher (system package)
sudo apt remove speech-dispatcher  # Ubuntu/Debian
sudo dnf remove speech-dispatcher  # Fedora
sudo pacman -R speech-dispatcher   # Arch Linux

# Remove espeak-ng (system package)
sudo apt remove espeak-ng  # Ubuntu/Debian
sudo dnf remove espeak-ng  # Fedora
sudo pacman -R espeak-ng   # Arch Linux
```

### Clean Build Directory
```bash
# Remove build artifacts
rm -rf builddir
```

## Getting Help

If you encounter issues during installation:

1. **Check system logs**: `journalctl --user -f`
2. **Enable debug mode**: Add `set tts_debug_mode true` to zathurarc
3. **Verify dependencies**: Use your distribution's package manager to check installed packages
4. **Report issues**: Create a GitHub issue with:
   - Your Linux distribution and version
   - Zathura version (`zathura --version`)
   - Build log output
   - Error messages

For additional help, consult the main README.md file or visit the project's GitHub page.