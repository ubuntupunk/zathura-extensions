# Troubleshooting Guide

This guide helps you diagnose and fix common issues with the Zathura TTS plugin.

## Quick Diagnostic Checklist

Before diving into specific issues, run through this quick checklist:

```bash
# 1. Check plugin installation
zathura --list-plugins | grep tts

# 2. Test TTS engines
echo "test" | piper --model en_US-lessac-medium 2>/dev/null && echo "Piper: OK" || echo "Piper: FAIL"
spd-say "test" 2>/dev/null && echo "Speech Dispatcher: OK" || echo "Speech Dispatcher: FAIL"
espeak-ng "test" 2>/dev/null && echo "espeak-ng: OK" || echo "espeak-ng: FAIL"

# 3. Check audio system
speaker-test -t wav -c 2 -l 1 >/dev/null 2>&1 && echo "Audio: OK" || echo "Audio: FAIL"

# 4. Check configuration
test -f ~/.config/zathura/zathurarc && echo "Config: EXISTS" || echo "Config: MISSING"
```

## Installation Issues

### Plugin Not Found

**Symptoms:**
- `zathura --list-plugins` doesn't show TTS plugin
- No TTS functionality in Zathura

**Diagnosis:**
```bash
# Check if plugin file exists
find /usr -name "*tts*" -type f 2>/dev/null
find ~/.local -name "*tts*" -type f 2>/dev/null

# Check Zathura plugin directory
zathura --print-plugin-dir
ls -la $(zathura --print-plugin-dir)
```

**Solutions:**

1. **Reinstall the plugin:**
   ```bash
   cd zathura-tts
   meson compile -C builddir
   sudo meson install -C builddir
   ```

2. **Check installation path:**
   ```bash
   # Plugin should be in one of these locations:
   ls -la /usr/lib/zathura/libtts.so
   ls -la /usr/local/lib/zathura/libtts.so
   ls -la ~/.local/lib/zathura/libtts.so
   ```

3. **Fix permissions:**
   ```bash
   sudo chmod 755 /usr/lib/zathura/libtts.so
   # OR for user installation:
   chmod 755 ~/.local/lib/zathura/libtts.so
   ```

### Build Failures

**Missing Dependencies:**
```bash
# Error: Package 'zathura' not found
sudo apt install zathura-dev libgirara-dev  # Ubuntu/Debian
sudo dnf install zathura-devel girara-devel  # Fedora
sudo pacman -S zathura girara  # Arch Linux

# Error: meson not found
pip3 install --user meson ninja
```

**Compilation Errors:**
```bash
# Error: girara/types.h not found
sudo apt install libgirara-dev  # Ubuntu/Debian

# Error: glib.h not found
sudo apt install libglib2.0-dev  # Ubuntu/Debian

# Error: gtk/gtk.h not found
sudo apt install libgtk-3-dev  # Ubuntu/Debian
```

## TTS Engine Issues

### No TTS Engine Available

**Symptoms:**
- Error message: "No TTS engine available"
- TTS shortcuts don't work

**Diagnosis:**
```bash
# Test each engine individually
which piper && echo "Piper installed" || echo "Piper missing"
which spd-say && echo "Speech Dispatcher installed" || echo "Speech Dispatcher missing"
which espeak-ng && echo "espeak-ng installed" || echo "espeak-ng missing"
```

**Solutions:**

1. **Install Piper-TTS (recommended):**
   ```bash
   pip3 install piper
   
   # Add to PATH if needed
   echo 'export PATH=$HOME/.local/bin:$PATH' >> ~/.bashrc
   source ~/.bashrc
   ```

2. **Install Speech Dispatcher:**
   ```bash
   sudo apt install speech-dispatcher  # Ubuntu/Debian
   sudo dnf install speech-dispatcher  # Fedora
   sudo pacman -S speech-dispatcher    # Arch Linux
   
   # Start the service
   systemctl --user start speech-dispatcher
   systemctl --user enable speech-dispatcher
   ```

3. **Install espeak-ng:**
   ```bash
   sudo apt install espeak-ng  # Ubuntu/Debian
   sudo dnf install espeak-ng  # Fedora
   sudo pacman -S espeak-ng    # Arch Linux
   ```

### Piper-TTS Issues

**Voice Not Found:**
```bash
# Error: "Voice not found: en_US-lessac-medium"

# Check available voices
piper --list-voices

# Download missing voice
mkdir -p ~/.local/share/zathura-tts/voices
cd ~/.local/share/zathura-tts/voices
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx.json
```

**Piper Crashes:**
```bash
# Test Piper directly
echo "test" | piper --model en_US-lessac-medium --debug

# Check Python installation
python3 -c "import piper; print('Piper module OK')"

# Reinstall if needed
pip3 uninstall piper
pip3 install piper
```

**Performance Issues:**
```bash
# Use smaller/faster voice models
# Download 'low' quality instead of 'medium' or 'high'
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/low/en_US-lessac-low.onnx
wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/low/en_US-lessac-low.onnx.json

# Configure in zathurarc
echo "set tts_piper_voice en_US-lessac-low" >> ~/.config/zathura/zathurarc
```

### Speech Dispatcher Issues

**Service Not Running:**
```bash
# Check service status
systemctl --user status speech-dispatcher

# Start service
systemctl --user start speech-dispatcher

# Enable automatic startup
systemctl --user enable speech-dispatcher

# If system service is needed:
sudo systemctl start speech-dispatcher
sudo systemctl enable speech-dispatcher
```

**No Voices Available:**
```bash
# List available voices
spd-say -L

# If empty, install voice packages
sudo apt install espeak-ng-data  # Ubuntu/Debian
sudo dnf install espeak-ng       # Fedora

# Test specific voice
spd-say -o espeak-ng -v en "test"
```

**Configuration Issues:**
```bash
# Reset Speech Dispatcher configuration
rm -rf ~/.config/speech-dispatcher
rm -rf ~/.speechd

# Reconfigure
speechd-conf
```

## Audio Issues

### No Audio Output

**Symptoms:**
- TTS appears to work (text highlighting, status changes)
- No sound is produced

**Diagnosis:**
```bash
# Test system audio
speaker-test -t wav -c 2 -l 1

# Check audio devices
aplay -l  # ALSA devices
pactl list sinks  # PulseAudio devices

# Test TTS engines directly
echo "test" | piper --model en_US-lessac-medium --output_file test.wav
aplay test.wav

spd-say "test"
espeak-ng "test"
```

**Solutions:**

1. **Fix PulseAudio:**
   ```bash
   # Restart PulseAudio
   pulseaudio -k
   pulseaudio --start
   
   # Check default sink
   pactl info | grep "Default Sink"
   
   # Set default sink if needed
   pactl set-default-sink alsa_output.pci-0000_00_1b.0.analog-stereo
   ```

2. **Fix ALSA:**
   ```bash
   # Check ALSA configuration
   cat /proc/asound/cards
   
   # Test ALSA directly
   aplay /usr/share/sounds/alsa/Front_Left.wav
   
   # Configure default device in ~/.asoundrc
   echo "defaults.pcm.card 0" > ~/.asoundrc
   echo "defaults.ctl.card 0" >> ~/.asoundrc
   ```

3. **Check permissions:**
   ```bash
   # Add user to audio group
   sudo usermod -a -G audio $USER
   
   # Logout and login again
   ```

### Audio Stuttering or Distortion

**Solutions:**
```bash
# Increase audio buffer size
echo "set tts_audio_buffer_size 8192" >> ~/.config/zathura/zathurarc

# Reduce audio quality for better performance
echo "set tts_audio_quality low" >> ~/.config/zathura/zathurarc

# Use different TTS engine
echo "set tts_engine espeak" >> ~/.config/zathura/zathurarc
```

## Performance Issues

### Slow Text Processing

**Symptoms:**
- Long delay before TTS starts
- Zathura becomes unresponsive during text extraction

**Solutions:**
```bash
# Use simpler text extraction
echo "set tts_extraction_method simple" >> ~/.config/zathura/zathurarc

# Disable reading order optimization
echo "set tts_optimize_reading_order false" >> ~/.config/zathura/zathurarc

# Reduce text buffer size
echo "set tts_text_buffer_size 1024" >> ~/.config/zathura/zathurarc
```

### High CPU Usage

**Solutions:**
```bash
# Use lighter TTS engine
echo "set tts_engine espeak" >> ~/.config/zathura/zathurarc

# Reduce speech quality
echo "set tts_piper_voice en_US-lessac-low" >> ~/.config/zathura/zathurarc

# Disable visual effects
echo "set tts_highlight_text false" >> ~/.config/zathura/zathurarc
echo "set tts_show_progress false" >> ~/.config/zathura/zathurarc
```

### Memory Issues

**Solutions:**
```bash
# Reduce memory usage
echo "set tts_text_cache_size 512" >> ~/.config/zathura/zathurarc
echo "set tts_audio_cache_size 1024" >> ~/.config/zathura/zathurarc

# Use streaming mode for large documents
echo "set tts_streaming_mode true" >> ~/.config/zathura/zathurarc
```

## Configuration Issues

### Settings Not Saved

**Diagnosis:**
```bash
# Check configuration file
ls -la ~/.config/zathura/zathurarc
cat ~/.config/zathura/zathurarc | grep tts

# Check directory permissions
ls -ld ~/.config/zathura/
```

**Solutions:**
```bash
# Create configuration directory
mkdir -p ~/.config/zathura

# Fix permissions
chmod 755 ~/.config/zathura
chmod 644 ~/.config/zathura/zathurarc

# Reset configuration
cp ~/.config/zathura/zathurarc ~/.config/zathura/zathurarc.backup
echo "# TTS Configuration" > ~/.config/zathura/zathurarc
echo "set tts_engine piper" >> ~/.config/zathura/zathurarc
```

### Invalid Configuration

**Symptoms:**
- Error messages about configuration
- TTS not working after configuration changes

**Solutions:**
```bash
# Validate configuration syntax
zathura --config-test ~/.config/zathura/zathurarc

# Check for common errors
grep -n "set tts" ~/.config/zathura/zathurarc

# Reset to defaults
cat > ~/.config/zathura/zathurarc << 'EOF'
# Basic TTS Configuration
set tts_engine piper
set tts_speed 1.0
set tts_volume 80
set tts_highlight_text true
set tts_highlight_color "#3498db"
EOF
```

## Text Extraction Issues

### No Text Found

**Symptoms:**
- Error: "No text found on page"
- TTS doesn't read anything

**Diagnosis:**
```bash
# Test text extraction manually
zathura --print-text document.pdf

# Check if PDF has selectable text
# Open PDF in Zathura and try to select text with mouse
```

**Solutions:**
```bash
# Try different extraction method
echo "set tts_extraction_method advanced" >> ~/.config/zathura/zathurarc

# For image-based PDFs, use OCR preprocessing
# (requires external OCR tools like tesseract)
```

### Garbled Text

**Solutions:**
```bash
# Improve text extraction
echo "set tts_text_cleanup true" >> ~/.config/zathura/zathurarc
echo "set tts_fix_encoding true" >> ~/.config/zathura/zathurarc

# Handle special characters
echo "set tts_unicode_support true" >> ~/.config/zathura/zathurarc
```

## Debug Mode

### Enable Detailed Logging

```bash
# Add debug settings to zathurarc
cat >> ~/.config/zathura/zathurarc << 'EOF'
# Debug settings
set tts_debug_mode true
set tts_log_level debug
set tts_log_file ~/.local/share/zathura-tts/debug.log
EOF

# Create log directory
mkdir -p ~/.local/share/zathura-tts

# Monitor logs in real-time
tail -f ~/.local/share/zathura-tts/debug.log
```

### Collect System Information

```bash
# Create system info report
cat > /tmp/tts-system-info.txt << EOF
=== System Information ===
OS: $(lsb_release -d 2>/dev/null || cat /etc/os-release | grep PRETTY_NAME)
Kernel: $(uname -r)
Architecture: $(uname -m)

=== Zathura Information ===
Version: $(zathura --version)
Plugins: $(zathura --list-plugins)
Plugin Dir: $(zathura --print-plugin-dir)

=== TTS Engines ===
Piper: $(which piper && piper --version || echo "Not installed")
Speech Dispatcher: $(which spd-say && spd-say --version || echo "Not installed")
espeak-ng: $(which espeak-ng && espeak-ng --version || echo "Not installed")

=== Audio System ===
PulseAudio: $(pulseaudio --version 2>/dev/null || echo "Not running")
ALSA: $(cat /proc/asound/version 2>/dev/null || echo "Not available")
Audio devices: $(aplay -l 2>/dev/null | grep card || echo "None found")

=== Configuration ===
$(cat ~/.config/zathura/zathurarc 2>/dev/null | grep tts || echo "No TTS configuration found")
EOF

echo "System information saved to /tmp/tts-system-info.txt"
cat /tmp/tts-system-info.txt
```

## Getting Help

If none of these solutions work:

1. **Enable debug mode** and collect logs
2. **Run the system information script** above
3. **Create a GitHub issue** with:
   - System information output
   - Debug log contents
   - Steps to reproduce the problem
   - Expected vs actual behavior

4. **Check existing issues** on GitHub for similar problems
5. **Join community discussions** for additional support

## Emergency Reset

If everything is broken and you want to start fresh:

```bash
# Remove all TTS configuration
sed -i '/tts/d' ~/.config/zathura/zathurarc

# Remove TTS data
rm -rf ~/.local/share/zathura-tts

# Reinstall plugin
cd zathura-tts
meson compile -C builddir
sudo meson install -C builddir

# Add minimal configuration
echo "set tts_engine espeak" >> ~/.config/zathura/zathurarc

# Test with simple engine
zathura document.pdf
# Press Ctrl+T to test
```

This should get you back to a working state with basic functionality.