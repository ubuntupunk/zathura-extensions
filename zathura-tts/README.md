# Zathura TTS Extension

[![Build Status](https://img.shields.io/github/actions/workflow/status/zathura-pdf/zathura-tts/ci.yml?branch=main&style=for-the-badge)](https://github.com/zathura-pdf/zathura-tts/actions)
[![Version](https://img.shields.io/github/v/release/zathura-pdf/zathura-tts?style=for-the-badge)](https://github.com/zathura-pdf/zathura-tts/releases)
[![License](https://img.shields.io/github/license/zathura-pdf/zathura-tts?style=for-the-badge)](https://www.zathura.org/license/)

A powerful text-to-speech (TTS) extension for the [Zathura PDF viewer](https://www.zathura.org/), designed to provide a seamless and feature-rich audio reading experience.

This plugin allows you to listen to your PDF documents directly within Zathura, with advanced controls for playback, voice customization, and navigation. It integrates with modern, high-quality TTS engines to deliver clear and natural-sounding speech.

## Features

- **Multiple TTS Engines**: Supports **Piper-TTS** (high-quality neural voices), **Speech Dispatcher** (system integration), and **espeak-ng** (reliable fallback).
- **Playback Control**: Play, pause, and stop audio narration with simple keyboard shortcuts.
- **Navigation**: Skip forward and backward by sentence or paragraph.
- **Voice Customization**: Adjust reading speed and select from available voices.
- **Visual Feedback**: Highlights the text currently being read.
- **Continuous Reading**: Automatically proceeds to the next page.
- **Special Content Handling**: Announces tables, lists, and other non-standard content.

*(demo.gif)*

## Requirements

### System Dependencies
- **Zathura**: Version 0.5.0 or higher
- **girara-gtk3**: Version 0.4.0 or higher
- **GLib**: Version 2.50 or higher
- **GTK+ 3**: Version 3.22 or higher
- **Speech Dispatcher** (optional, for system TTS)

### Python Dependencies
- **Python**: Version 3.8 or higher
- **piper-tts**: Version 1.2.0 or higher

## Installation

1.  **Install System Dependencies**:
    Ensure you have Zathura and its development libraries installed. If you want to use the system's TTS, install Speech Dispatcher.

    ```bash
    # Example for Arch Linux
    sudo pacman -S zathura zathura-devel girara speech-dispatcher
    ```

2.  **Install Piper-TTS**:
    For the best audio quality, install the Piper-TTS Python package.

    ```bash
    pip install piper
    ```

3.  **Build the Plugin**:
    Clone the repository and use Meson to build and install the plugin.

    ```bash
    git clone https://github.com/zathura-pdf/zathura-tts.git
    cd zathura-tts
    meson setup build
    meson compile -C build
    sudo meson install -C build
    ```

4.  **Download Piper Voices**:
    Piper requires voice models to function. You can download high-quality voices from the [Piper Voices repository on Hugging Face](https://huggingface.co/rhasspy/piper-voices/tree/main).

    Each voice consists of a `.onnx` file and a `.onnx.json` file.

    **Example**: To download the `en_US-lessac-medium` voice:
    ```bash
    wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx
    wget https://huggingface.co/rhasspy/piper-voices/resolve/main/en/en_US/lessac/medium/en_US-lessac-medium.onnx.json
    ```

    You should place these voice files in a directory where the plugin can find them, such as `~/.local/share/zathura-tts/voices/`. You can configure the voice path in your `zathurarc`.

## Usage

### Getting Started

1. **Open a PDF in Zathura:**
   ```bash
   zathura document.pdf
   ```

2. **Start TTS reading:**
   - Press `Ctrl+T` to start reading from the current page
   - The plugin will automatically detect and use the best available TTS engine

3. **Control playback:**
   - Use keyboard shortcuts for full control over the reading experience
   - Visual feedback shows the currently spoken text

### Keyboard Shortcuts

#### Primary Controls
| Shortcut | Action | Description |
|----------|--------|-------------|
| `Ctrl+T` | Toggle TTS | Start reading from current position or stop current reading |
| `Ctrl+Space` | Pause/Resume | Pause or resume current TTS playback |
| `Ctrl+Shift+T` | Settings | Open TTS configuration dialog |

#### Navigation Controls
| Shortcut | Action | Description |
|----------|--------|-------------|
| `Ctrl+Right` | Next Sentence | Skip to next sentence |
| `Ctrl+Left` | Previous Sentence | Go back to previous sentence |
| `Ctrl+Down` | Next Paragraph | Skip to next paragraph |
| `Ctrl+Up` | Previous Paragraph | Go back to previous paragraph |
| `Ctrl+Page Down` | Next Page | Continue reading on next page |
| `Ctrl+Page Up` | Previous Page | Go back to previous page |

#### Speed and Volume Controls
| Shortcut | Action | Description |
|----------|--------|-------------|
| `Ctrl++` | Increase Speed | Make speech faster (up to 3.0x) |
| `Ctrl+-` | Decrease Speed | Make speech slower (down to 0.5x) |
| `Ctrl+0` | Reset Speed | Return to default speed (1.0x) |
| `Ctrl+Shift++` | Increase Volume | Make speech louder |
| `Ctrl+Shift+-` | Decrease Volume | Make speech quieter |

### Reading Modes

#### Continuous Reading
- Starts from current cursor position
- Automatically continues through pages
- Maintains reading position across sessions
- Ideal for reading entire documents

#### Page Reading
- Reads only the current page
- Useful for reviewing specific content
- Automatically stops at page end
- Good for focused reading sessions

#### Selection Reading
- Select text with mouse first
- Press `Ctrl+T` to read selection only
- Perfect for reading specific passages
- Maintains selection highlighting

### Visual Feedback

The plugin provides several visual indicators:

- **Text Highlighting**: Currently spoken text is highlighted with a colored background
- **Status Indicator**: Shows current TTS state (Playing, Paused, Stopped) in the status bar
- **Progress Indicator**: Displays reading progress for long documents
- **Engine Indicator**: Shows which TTS engine is currently active

## Configuration

### Configuration Methods

The plugin can be configured in two ways:

1. **Settings Dialog**: Press `Ctrl+Shift+T` for interactive configuration
2. **Configuration File**: Edit `~/.config/zathura/zathurarc` for persistent settings

### Configuration Options

#### TTS Engine Settings
```bash
# Preferred TTS engine (piper, speech-dispatcher, espeak)
set tts_engine piper

# Enable automatic fallback to other engines
set tts_auto_fallback true

# Engine-specific voice selection
set tts_piper_voice en_US-lessac-medium
set tts_speechd_voice default
set tts_espeak_voice en
```

#### Speech Settings
```bash
# Speech rate (0.5 - 3.0)
set tts_speed 1.0

# Volume level (0 - 100)
set tts_volume 80

# Pitch adjustment (-50 to 50, Piper only)
set tts_pitch 0

# Pause between sentences (milliseconds)
set tts_sentence_pause 500

# Pause between paragraphs (milliseconds)
set tts_paragraph_pause 1000
```

#### Visual Settings
```bash
# Enable text highlighting during reading
set tts_highlight_text true

# Highlight color (hex format)
set tts_highlight_color "#3498db"

# Show TTS status in status bar
set tts_show_status true

# Show reading progress indicator
set tts_show_progress true
```

#### Advanced Settings
```bash
# Text extraction method (auto, simple, advanced)
set tts_extraction_method auto

# Announce mathematical formulas
set tts_announce_math true

# Announce table structure
set tts_announce_tables true

# Announce hyperlinks
set tts_announce_links true

# Optimize reading order for better flow
set tts_optimize_reading_order true
```

### Voice Configuration

#### Piper-TTS Voices

1. **List available voices:**
   ```bash
   piper --list-voices
   ```

2. **Download additional voices:**
   ```bash
   # Create voice directory
   mkdir -p ~/.local/share/zathura-tts/voices
   
   # Download voice files (example for German)
   cd ~/.local/share/zathura-tts/voices
   wget https://huggingface.co/rhasspy/piper-voices/resolve/main/de/de_DE/thorsten/medium/de_DE-thorsten-medium.onnx
   wget https://huggingface.co/rhasspy/piper-voices/resolve/main/de/de_DE/thorsten/medium/de_DE-thorsten-medium.onnx.json
   ```

3. **Configure voice path:**
   ```bash
   set tts_piper_voice_path ~/.local/share/zathura-tts/voices/
   ```

#### Speech Dispatcher Voices

1. **List available voices:**
   ```bash
   spd-say -L
   ```

2. **Test voice:**
   ```bash
   spd-say -o espeak-ng -v en "Hello world"
   ```

3. **Configure in zathurarc:**
   ```bash
   set tts_speechd_voice espeak-ng
   set tts_speechd_language en
   ```

### Runtime Configuration

You can also modify settings while Zathura is running using command mode:

```bash
# Press ':' to enter command mode, then use:
:set tts_speed 1.5          # Set speech speed
:set tts_volume 90          # Set volume
:set tts_voice en_US-amy    # Change voice
:set tts_engine speechd     # Switch TTS engine
```

## Troubleshooting

### Common Issues

#### TTS Not Working

**Problem**: No speech output when pressing TTS shortcuts

**Diagnostic Steps**:
1. **Check plugin installation:**
   ```bash
   zathura --list-plugins | grep tts
   ```

2. **Test TTS engines individually:**
   ```bash
   # Test Piper
   echo "Hello world" | piper --model en_US-lessac-medium
   
   # Test Speech Dispatcher
   spd-say "Hello world"
   
   # Test espeak-ng
   espeak-ng "Hello world"
   ```

3. **Check configuration file:**
   ```bash
   cat ~/.config/zathura/zathurarc | grep tts
   ```

**Solutions**:
- Install missing TTS engines
- Verify configuration syntax
- Check file permissions on config directory

#### Audio Issues

**Problem**: TTS starts but no audio is heard

**Diagnostic Steps**:
1. **Test system audio:**
   ```bash
   speaker-test -t wav -c 2
   ```

2. **Check audio system:**
   ```bash
   # PulseAudio
   pulseaudio --check -v
   
   # ALSA
   aplay /usr/share/sounds/alsa/Front_Left.wav
   ```

3. **Check TTS engine audio output:**
   ```bash
   # Test with direct engine calls
   piper --model en_US-lessac-medium --output_file test.wav < /dev/null
   aplay test.wav
   ```

**Solutions**:
- Restart audio system: `pulseaudio -k && pulseaudio --start`
- Check audio device selection in TTS engine settings
- Verify audio permissions for user account

#### Performance Issues

**Problem**: TTS is slow or causes Zathura to freeze

**Solutions**:
1. **Reduce text processing complexity:**
   ```bash
   set tts_extraction_method simple
   set tts_optimize_reading_order false
   ```

2. **Use lighter TTS engine:**
   ```bash
   set tts_engine espeak
   ```

3. **Adjust buffer settings:**
   ```bash
   set tts_text_buffer_size 1024
   set tts_audio_buffer_size 4096
   ```

#### Voice Quality Issues

**Problem**: Voice sounds robotic or unclear

**Solutions**:
1. **Switch to higher quality engine:**
   ```bash
   set tts_engine piper
   ```

2. **Download better quality voices for Piper**

3. **Adjust speech parameters:**
   ```bash
   set tts_speed 0.9
   set tts_pitch 5
   ```

### Error Messages

#### "No TTS engine available"
- **Cause**: No supported TTS engines are installed
- **Solution**: Install at least one: `pip install piper` or `sudo apt install speech-dispatcher espeak-ng`

#### "Voice not found: [voice_name]"
- **Cause**: Specified voice is not available
- **Solution**: Check available voices with `piper --list-voices` or `spd-say -L`

#### "Configuration file error"
- **Cause**: Syntax error in zathurarc
- **Solution**: Check configuration syntax, reset by removing problematic lines

#### "Text extraction failed"
- **Cause**: PDF has no selectable text or is image-based
- **Solution**: Try different extraction method or use OCR-processed PDFs

#### "Audio device not found"
- **Cause**: Audio system configuration issue
- **Solution**: Check audio device settings, restart audio system

### Debug Mode

Enable detailed logging for troubleshooting:

1. **Enable debug logging:**
   ```bash
   set tts_debug_mode true
   set tts_log_file ~/.local/share/zathura-tts/debug.log
   ```

2. **View debug output:**
   ```bash
   tail -f ~/.local/share/zathura-tts/debug.log
   ```

3. **Common debug information includes:**
   - TTS engine detection and initialization
   - Text extraction process
   - Audio system interaction
   - Configuration loading and validation

### Getting Help

If issues persist:

1. **Check system information:**
   ```bash
   zathura --version
   piper --version
   spd-say --version
   ```

2. **Gather debug information:**
   - Enable debug mode
   - Reproduce the issue
   - Collect log output

3. **Report issues:**
   - Create GitHub issue with system info and logs
   - Include configuration file contents
   - Describe steps to reproduce the problem

## Supported TTS Engines

The plugin selects the best available TTS engine in the following order:

1.  **Piper-TTS**: A high-quality, fast, and local neural text-to-speech system. It offers the most natural-sounding voices and is the recommended engine.
2.  **Speech Dispatcher**: A common interface for system-level TTS services on Linux. It provides access to a wide range of voices that may already be installed on your system.
3.  **espeak-ng**: A compact and reliable software speech synthesizer. It serves as a fallback and works on most systems without extra configuration.

## Development

For development, you can build the plugin without installing it system-wide.

```bash
meson setup build
meson compile -C build
```

To run tests:
```bash
meson test -C build
```

## Roadmap

- [ ] Enhanced handling of scientific and mathematical notation.
- [ ] Support for more TTS engines and platforms.
- [ ] Improved UI for settings and voice selection.
- [ ] End-to-end testing with various document types.

## Contributing

Contributions are welcome! Please feel free to submit a pull request or open an issue for bugs, feature requests, or suggestions.

## License

This project is licensed under the Zlib license. See the [LICENSE](LICENSE) file for details.
