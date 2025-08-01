# Zathura TTS Plugin

Text-to-speech functionality for the Zathura PDF viewer.

## Features

- Read PDF content aloud using various TTS engines
- Support for Piper-TTS, Speech Dispatcher, and espeak-ng
- Keyboard shortcuts for TTS control
- Configurable voice settings and reading speed
- Visual highlighting of currently spoken text

## Building

This plugin requires:
- Zathura >= 0.5.0
- girara-gtk3 >= 0.4.0
- GLib >= 2.50
- GTK+ 3 >= 3.22

Optional dependencies:
- Speech Dispatcher (for system TTS integration)

Build with meson:

```bash
meson setup build
meson compile -C build
meson install -C build
```

## Usage

After installation, the TTS functionality will be available in Zathura:

- `Ctrl+T`: Toggle TTS on/off
- `Ctrl+Space`: Pause/resume
- `Ctrl+Right`: Next sentence
- `Ctrl+Left`: Previous sentence

## License

This project is licensed under the same terms as Zathura (Zlib license).