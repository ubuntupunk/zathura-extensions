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

### ⚠️ **Important: Modified Zathura Required**

This TTS plugin requires a **modified version of Zathura** with utility plugin support. The standard Zathura from package managers will **not work**.

**Two options to get the required Zathura:**

1. **Use our pre-built version** (included as submodule):
   ```bash
   git clone --recursive https://github.com/ubuntupunk/zathura-liberated.git
   cd zathura-liberated
   # Build and install modified Zathura (see Installation section)
   ```

2. **Apply our patch to upstream Zathura**:
   ```bash
   git clone https://github.com/pwmt/zathura.git
   cd zathura
   git apply ../0001-Add-utility-plugin-support-and-TTS-API-functions.patch
   # Build and install
   ```

### System Dependencies
- **Modified Zathura**: With utility plugin support (see above)
- **girara-gtk3**: Version 0.4.0 or higher
- **GLib**: Version 2.50 or higher
- **GTK+ 3**: Version 3.22 or higher
- **Speech Dispatcher** (optional, for system TTS)

### Python Dependencies
- **Python**: Version 3.8 or higher
- **piper-tts**: Version 1.2.0 or higher

## Installation

### Step 1: Build Modified Zathura

Since this plugin requires a modified version of Zathura, you must build and install it first:

```bash
# Clone with submodules
git clone --recursive https://github.com/ubuntupunk/zathura-liberated.git
cd zathura-liberated

# Build and install modified Zathura
cd zathura
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir

# Build and install PDF plugin
cd ../zathura-pdf-poppler
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir

cd ..
```

### Step 2: Install System Dependencies

```bash
# Example for Debian/Ubuntu
sudo apt install libgirara-dev libgtk-3-dev libglib2.0-dev speech-dispatcher

# Example for Arch Linux
sudo pacman -S girara gtk3 glib2 speech-dispatcher
```

### Step 3: Install Piper-TTS
For the best audio quality, install the Piper-TTS Python package.

```bash
pip install piper
```

### Step 4: Build the TTS Plugin
Build and install the TTS plugin:

```bash
# From the zathura-liberated directory
cd zathura-tts
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir
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

Once installed, the TTS functionality can be controlled with the following keyboard shortcuts in Zathura:

| Shortcut          | Action                  |
| ----------------- | ----------------------- |
| `Ctrl+T`          | Toggle TTS on/off       |
| `Ctrl+Space`      | Pause/Resume reading    |
| `Ctrl+Right`      | Skip to the next sentence |
| `Ctrl+Left`       | Go to the previous sentence |
| `Ctrl+Shift+T`    | Open TTS settings       |

## Configuration

The plugin can be configured by editing the `zathurarc` file or through the TTS settings dialog (`Ctrl+Shift+T`).

Available options include:
- **`tts_engine`**: `piper`, `speech-dispatcher`, or `espeak` (default: `piper`).
- **`tts_voice`**: Specify a voice for the selected engine.
- **`tts_speed`**: Reading speed multiplier (0.5x to 3.0x).
- **`tts_highlight_color`**: Color for highlighting spoken text.

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
