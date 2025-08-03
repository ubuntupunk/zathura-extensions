# Quick Start Guide

Get up and running with Zathura TTS in just a few minutes!

## 5-Minute Setup

### Step 1: Install Prerequisites (2 minutes)

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install zathura zathura-dev libgirara-dev meson ninja-build
pip3 install piper
```

**Fedora:**
```bash
sudo dnf install zathura zathura-devel girara-devel meson ninja-build
pip3 install piper
```

**Arch Linux:**
```bash
sudo pacman -S zathura girara meson ninja
pip install --user piper
```

### Step 2: Build and Install (2 minutes)

```bash
# Clone and build
git clone https://github.com/zathura-pdf/zathura-tts.git
cd zathura-tts
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir
```

### Step 3: Download a Voice (1 minute)

```bash
# Create voice directory
mkdir -p ~/.local/share/zathura-tts/voices
cd ~/.local/share/zathura-tts/voices

# Download English voice
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx.json
```

### Step 4: Test It Out!

```bash
# Open any PDF
zathura sample.pdf

# Press Ctrl+T to start reading
# Press Space to pause/resume
# Press Ctrl+T again to stop
```

## Essential Keyboard Shortcuts

| Key | Action |
|-----|--------|
| `Ctrl+T` | Start/Stop reading |
| `Space` | Pause/Resume |
| `Ctrl+Right` | Next sentence |
| `Ctrl+Left` | Previous sentence |
| `Ctrl++` | Faster speech |
| `Ctrl+-` | Slower speech |
| `Ctrl+Shift+T` | Settings |

## First-Time Configuration

### Basic Settings
Add these lines to `~/.config/zathura/zathurarc`:

```bash
# Essential TTS settings
set tts_engine piper
set tts_speed 1.0
set tts_volume 80
set tts_highlight_text true
```

### Choose Your Voice
```bash
# List available Piper voices
piper --list-voices

# Set your preferred voice
echo "set tts_piper_voice en_US-lessac-medium" >> ~/.config/zathura/zathurarc
```

## Common First-Time Issues

### "No TTS engine available"
**Quick Fix:**
```bash
# Install at least one TTS engine
pip3 install piper
# OR
sudo apt install espeak-ng
```

### "Voice not found"
**Quick Fix:**
```bash
# Download the default voice
mkdir -p ~/.local/share/zathura-tts/voices
cd ~/.local/share/zathura-tts/voices
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx.json
```

### "No audio output"
**Quick Fix:**
```bash
# Test your audio system
speaker-test -t wav -c 2

# If no sound, restart audio
pulseaudio -k && pulseaudio --start
```

## Next Steps

Once you have TTS working:

1. **Explore more voices**: Browse [Piper Voices](https://huggingface.co/rhasspy/piper-voices/tree/main) for different languages and styles
2. **Customize settings**: Press `Ctrl+Shift+T` to open the settings dialog
3. **Learn advanced shortcuts**: Check the full documentation for navigation and control options
4. **Optimize performance**: Adjust settings for your system in the configuration file

## Getting Help

- **Full documentation**: See `README.md` and `docs/INSTALLATION.md`
- **Troubleshooting**: Check the troubleshooting section in the main README
- **Report issues**: Create a GitHub issue with your system details

Happy reading! 🎧📖