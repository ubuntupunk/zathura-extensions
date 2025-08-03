# Developer Documentation

This guide provides comprehensive information for developers who want to understand, modify, or extend the Zathura TTS plugin.

## Table of Contents

1. [Architecture Overview](#architecture-overview)
2. [Plugin Structure](#plugin-structure)
3. [Core Components](#core-components)
4. [Extension Points](#extension-points)
5. [Development Setup](#development-setup)
6. [Building and Testing](#building-and-testing)
7. [API Reference](#api-reference)
8. [Contributing Guidelines](#contributing-guidelines)

## Architecture Overview

The Zathura TTS plugin follows a modular architecture with clear separation of concerns:

```
┌─────────────────────────────────────────────────────────────┐
│                    Zathura PDF Viewer                      │
├─────────────────────────────────────────────────────────────┤
│                    Plugin Interface                        │
├─────────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │     UI      │  │    Text     │  │    Configuration    │  │
│  │ Controller  │  │ Extractor   │  │     Manager         │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   Audio     │  │    TTS      │  │       Error         │  │
│  │ Controller  │  │   Engine    │  │      Handler        │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
├─────────────────────────────────────────────────────────────┤
│                    TTS Engines                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐  │
│  │   Piper     │  │   Speech    │  │      espeak-ng      │  │
│  │    TTS      │  │ Dispatcher  │  │                     │  │
│  └─────────────┘  └─────────────┘  └─────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
```

### Design Principles

1. **Modularity**: Each component has a single responsibility
2. **Extensibility**: Easy to add new TTS engines or features
3. **Error Resilience**: Graceful degradation when components fail
4. **Performance**: Efficient text processing and audio handling
5. **User Experience**: Intuitive controls and visual feedback

## Plugin Structure

```
zathura-tts/
├── src/                          # Source code
│   ├── plugin.c                  # Main plugin entry point
│   ├── plugin.h                  # Plugin interface definitions
│   ├── tts-audio-controller.c    # Audio state management
│   ├── tts-audio-controller.h
│   ├── tts-config.c              # Configuration management
│   ├── tts-config.h
│   ├── tts-engine.c              # TTS engine abstraction
│   ├── tts-engine.h
│   ├── tts-error.c               # Error handling system
│   ├── tts-error.h
│   ├── tts-text-extractor.c      # Text extraction from PDFs
│   ├── tts-text-extractor.h
│   ├── tts-ui-controller.c       # UI integration and shortcuts
│   └── tts-ui-controller.h
├── tests/                        # Test suite
│   ├── test-framework.c          # Testing framework
│   ├── test-framework.h
│   ├── test-simple.c             # Unit tests
│   ├── test-integration-simple.c # Integration tests
│   └── test-*.c                  # Additional test files
├── docs/                         # Documentation
│   ├── DEVELOPER.md              # This file
│   ├── INSTALLATION.md           # Installation guide
│   ├── QUICKSTART.md             # Quick start guide
│   └── TROUBLESHOOTING.md        # Troubleshooting guide
├── data/                         # Desktop files and metadata
├── voices/                       # Sample voice files
├── meson.build                   # Build configuration
├── meson_options.txt             # Build options
└── README.md                     # Main documentation
```

## Core Components

### 1. Plugin Core (`plugin.c`, `plugin.h`)

The main plugin interface that integrates with Zathura.

**Key Functions:**
- `tts_plugin_register()`: Registers the plugin with Zathura
- `tts_plugin_init()`: Initializes all plugin components
- `tts_plugin_cleanup()`: Cleans up resources on shutdown

**Integration Points:**
- Zathura plugin API
- Girara UI framework
- GTK+ event system

### 2. Text Extractor (`tts-text-extractor.c`)

Extracts and processes text from PDF documents.

**Key Features:**
- Page text extraction using Zathura's document API
- Text segmentation (sentences, paragraphs)
- Reading order optimization
- Special content handling (math, tables, links)

**Extension Points:**
- Custom text processing filters
- Additional content type handlers
- Alternative extraction algorithms

### 3. TTS Engine Interface (`tts-engine.c`)

Provides a unified interface for multiple TTS engines.

**Supported Engines:**
- **Piper-TTS**: High-quality neural voices
- **Speech Dispatcher**: System TTS integration
- **espeak-ng**: Lightweight fallback

**Engine Interface:**
```c
typedef struct {
    char* name;
    bool (*is_available)(void);
    bool (*initialize)(tts_config_t* config);
    bool (*speak)(const char* text);
    bool (*stop)(void);
    bool (*set_speed)(float speed);
    bool (*set_volume)(int volume);
    void (*cleanup)(void);
} tts_engine_interface_t;
```

### 4. Audio Controller (`tts-audio-controller.c`)

Manages TTS playback state and audio controls.

**State Machine:**
```
STOPPED ──→ PLAYING ──→ PAUSED
   ↑           ↓           ↓
   └───────────┴───────────┘
```

**Key Features:**
- Thread-safe state management
- Speed and volume control
- Navigation between text segments
- Session persistence

### 5. UI Controller (`tts-ui-controller.c`)

Handles keyboard shortcuts and visual feedback.

**Keyboard Shortcuts:**
- Primary controls (start/stop, pause/resume)
- Navigation (sentence/paragraph jumping)
- Settings (speed, volume adjustment)

**Visual Feedback:**
- Text highlighting during reading
- Status indicators
- Progress display

### 6. Configuration Manager (`tts-config.c`)

Manages plugin settings and preferences.

**Configuration Sources:**
1. Default values (hardcoded)
2. System configuration files
3. User configuration files
4. Runtime settings

**Configuration Flow:**
```
Defaults → System Config → User Config → Runtime → Active Config
```

### 7. Error Handler (`tts-error.c`)

Provides comprehensive error handling and user feedback.

**Error Categories:**
- Engine errors (TTS engine failures)
- Configuration errors (invalid settings)
- System errors (audio, file I/O)
- User errors (invalid input)

## Extension Points

### Adding New TTS Engines

1. **Create Engine Implementation:**
```c
// tts-engine-myengine.c
static bool myengine_is_available(void) {
    // Check if engine is installed
    return system("which myengine > /dev/null 2>&1") == 0;
}

static bool myengine_speak(const char* text) {
    // Implement text-to-speech
    char command[1024];
    snprintf(command, sizeof(command), "myengine \"%s\"", text);
    return system(command) == 0;
}

// Implement other interface functions...

tts_engine_interface_t myengine_interface = {
    .name = "myengine",
    .is_available = myengine_is_available,
    .speak = myengine_speak,
    // ... other functions
};
```

2. **Register Engine:**
```c
// In tts-engine.c
extern tts_engine_interface_t myengine_interface;

static tts_engine_interface_t* available_engines[] = {
    &piper_interface,
    &speechd_interface,
    &espeak_interface,
    &myengine_interface,  // Add your engine
    NULL
};
```

### Adding New Text Processors

1. **Create Processor:**
```c
// tts-text-processor-custom.c
char* custom_process_text(const char* input, tts_config_t* config) {
    // Custom text processing logic
    char* processed = malloc(strlen(input) * 2);
    // ... processing logic
    return processed;
}
```

2. **Register Processor:**
```c
// In tts-text-extractor.c
typedef struct {
    char* name;
    char* (*process)(const char* input, tts_config_t* config);
} text_processor_t;

static text_processor_t processors[] = {
    {"default", default_process_text},
    {"custom", custom_process_text},  // Add your processor
    {NULL, NULL}
};
```

### Adding New Configuration Options

1. **Define Option:**
```c
// In tts-config.h
typedef struct {
    // ... existing fields
    bool my_custom_option;
} tts_config_t;
```

2. **Add Getter/Setter:**
```c
// In tts-config.c
bool tts_config_get_my_custom_option(tts_config_t* config) {
    return config ? config->my_custom_option : false;
}

bool tts_config_set_my_custom_option(tts_config_t* config, bool value) {
    if (!config) return false;
    config->my_custom_option = value;
    config->modified = true;
    return true;
}
```

3. **Add to Configuration File Handling:**
```c
// In configuration parsing code
if (strcmp(key, "my_custom_option") == 0) {
    config->my_custom_option = parse_boolean(value);
}
```

## Development Setup

### Prerequisites

**System Dependencies:**
```bash
# Ubuntu/Debian
sudo apt install build-essential meson ninja-build pkg-config
sudo apt install zathura-dev libgirara-dev libglib2.0-dev libgtk-3-dev

# Development tools
sudo apt install gdb valgrind clang-format git
```

**TTS Engines (for testing):**
```bash
pip install piper
sudo apt install speech-dispatcher espeak-ng
```

### Environment Setup

1. **Clone Repository:**
```bash
git clone https://github.com/zathura-pdf/zathura-tts.git
cd zathura-tts
```

2. **Set up Development Environment:**
```bash
# Create development build
meson setup builddir-dev --buildtype=debug

# Enable all warnings and debug symbols
meson configure builddir-dev -Dwarning_level=3 -Ddebug=true
```

3. **Configure IDE/Editor:**

**VS Code Configuration (`.vscode/settings.json`):**
```json
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.compileCommands": "builddir-dev/compile_commands.json",
    "files.associations": {
        "*.h": "c",
        "*.c": "c"
    }
}
```

**Vim Configuration:**
```vim
" Add to .vimrc for C development
set path+=src/,tests/
set tags=./tags,tags
```

## Building and Testing

### Development Build

```bash
# Debug build with all warnings
meson setup builddir-dev --buildtype=debug -Dwarning_level=3

# Compile
meson compile -C builddir-dev

# Install to local directory for testing
meson install -C builddir-dev --destdir=./install-test
```

### Running Tests

```bash
# Run all tests
meson test -C builddir-dev

# Run specific test suite
meson test -C builddir-dev unit-tests
meson test -C builddir-dev integration-tests

# Run tests with verbose output
meson test -C builddir-dev --verbose

# Run tests under valgrind (memory checking)
meson test -C builddir-dev --wrap='valgrind --leak-check=full'
```

### Manual Testing

```bash
# Test with development build
export ZATHURA_PLUGIN_PATH=./builddir-dev/src
zathura test-document.pdf

# Test specific functionality
./builddir-dev/tests/test-simple-runner
./builddir-dev/tests/test-integration-simple-runner
```

### Code Quality Tools

**Static Analysis:**
```bash
# Run static analysis with clang
clang-static-analyzer src/*.c

# Check code formatting
clang-format --dry-run --Werror src/*.c src/*.h
```

**Memory Checking:**
```bash
# Run with valgrind
valgrind --tool=memcheck --leak-check=full zathura document.pdf

# Run tests with AddressSanitizer
meson configure builddir-dev -Db_sanitize=address
meson compile -C builddir-dev
meson test -C builddir-dev
```

**Code Coverage:**
```bash
# Enable coverage
meson configure builddir-dev -Db_coverage=true
meson compile -C builddir-dev
meson test -C builddir-dev

# Generate coverage report
ninja -C builddir-dev coverage-html
# View report in builddir-dev/meson-logs/coveragereport/index.html
```

## API Reference

### Plugin Interface

```c
// Main plugin functions
zathura_error_t tts_plugin_register(zathura_t* zathura);
zathura_error_t tts_plugin_init(zathura_t* zathura);
void tts_plugin_cleanup(void);

// Plugin state
bool tts_plugin_is_initialized(void);
tts_plugin_t* tts_plugin_get_instance(void);
```

### Text Extraction API

```c
// Extract text from current page
char* tts_extract_page_text(zathura_page_t* page);

// Extract text from selection
char* tts_extract_selection_text(zathura_page_t* page, 
                                 zathura_rectangle_t selection);

// Text segmentation
char** tts_segment_text_sentences(const char* text);
char** tts_segment_text_paragraphs(const char* text);

// Text processing
char* tts_process_text(const char* text, tts_config_t* config);
```

### TTS Engine API

```c
// Engine management
tts_engine_t* tts_engine_get_current(void);
bool tts_engine_set_current(const char* engine_name);
char** tts_engine_list_available(void);

// Speech control
bool tts_engine_speak(const char* text);
bool tts_engine_stop(void);
bool tts_engine_pause(void);
bool tts_engine_resume(void);

// Settings
bool tts_engine_set_speed(float speed);
bool tts_engine_set_volume(int volume);
bool tts_engine_set_voice(const char* voice);
```

### Audio Controller API

```c
// State management
tts_audio_state_t tts_audio_controller_get_state(tts_audio_controller_t* controller);
bool tts_audio_controller_set_state(tts_audio_controller_t* controller, 
                                    tts_audio_state_t state);

// Playback control
bool tts_audio_controller_play(tts_audio_controller_t* controller, const char* text);
bool tts_audio_controller_pause(tts_audio_controller_t* controller);
bool tts_audio_controller_stop(tts_audio_controller_t* controller);

// Navigation
bool tts_audio_controller_next_sentence(tts_audio_controller_t* controller);
bool tts_audio_controller_previous_sentence(tts_audio_controller_t* controller);

// Settings
bool tts_audio_controller_set_speed(tts_audio_controller_t* controller, float speed);
bool tts_audio_controller_set_volume(tts_audio_controller_t* controller, int volume);
```

### Configuration API

```c
// Configuration management
tts_config_t* tts_config_new(void);
void tts_config_free(tts_config_t* config);
bool tts_config_load_from_file(tts_config_t* config, const char* filename);
bool tts_config_save_to_file(tts_config_t* config, const char* filename);

// Settings access
float tts_config_get_default_speed(tts_config_t* config);
bool tts_config_set_default_speed(tts_config_t* config, float speed);
int tts_config_get_default_volume(tts_config_t* config);
bool tts_config_set_default_volume(tts_config_t* config, int volume);

// Validation
bool tts_config_validate(tts_config_t* config, char** error_message);
```

### Error Handling API

```c
// Error context management
tts_error_context_t* tts_error_context_new(tts_error_code_t code,
                                           tts_error_severity_t severity,
                                           const char* message,
                                           const char* details,
                                           const char* component,
                                           const char* function,
                                           int line);
void tts_error_context_free(tts_error_context_t* context);

// Error information
const char* tts_error_get_string(tts_error_code_t code);
char* tts_error_get_user_message(tts_error_code_t code, const char* details);
bool tts_error_should_retry(tts_error_code_t code);
bool tts_error_is_recoverable(tts_error_code_t code);
```

## Contributing Guidelines

### Code Style

**Formatting:**
- Use 4 spaces for indentation (no tabs)
- Maximum line length: 100 characters
- Use clang-format with the provided configuration

**Naming Conventions:**
- Functions: `tts_component_action()` (snake_case with prefix)
- Types: `tts_component_t` (snake_case with _t suffix)
- Constants: `TTS_CONSTANT_NAME` (UPPER_CASE with prefix)
- Variables: `variable_name` (snake_case)

**Example:**
```c
// Good
bool tts_audio_controller_set_speed(tts_audio_controller_t* controller, float speed);

// Bad
bool SetSpeed(AudioController* ctrl, float s);
```

### Documentation

**Function Documentation:**
```c
/**
 * Sets the playback speed for the audio controller.
 *
 * @param controller The audio controller instance
 * @param speed The playback speed (0.5 - 3.0)
 * @return true if speed was set successfully, false otherwise
 *
 * @note Speed values outside the valid range will be clamped
 * @since 1.0.0
 */
bool tts_audio_controller_set_speed(tts_audio_controller_t* controller, float speed);
```

**Header Comments:**
```c
/**
 * @file tts-audio-controller.h
 * @brief Audio controller for TTS playback management
 * @author Your Name
 * @date 2024
 * @copyright MIT License
 */
```

### Testing Requirements

**Unit Tests:**
- Every public function must have unit tests
- Test both success and failure cases
- Use descriptive test names

**Integration Tests:**
- Test component interactions
- Test complete workflows
- Test error handling paths

**Example Test:**
```c
static void test_audio_controller_speed_validation(void) {
    TEST_CASE_BEGIN("Audio Controller Speed Validation");
    
    tts_audio_controller_t* controller = tts_audio_controller_new();
    TEST_ASSERT_NOT_NULL(controller, "Controller should be created");
    
    // Test valid speed
    bool result = tts_audio_controller_set_speed(controller, 1.5f);
    TEST_ASSERT(result, "Valid speed should be accepted");
    
    // Test invalid speed
    result = tts_audio_controller_set_speed(controller, 10.0f);
    TEST_ASSERT(!result, "Invalid speed should be rejected");
    
    tts_audio_controller_free(controller);
    TEST_CASE_END();
}
```

### Pull Request Process

1. **Fork and Branch:**
```bash
git fork https://github.com/zathura-pdf/zathura-tts.git
git checkout -b feature/my-new-feature
```

2. **Development:**
- Write code following style guidelines
- Add comprehensive tests
- Update documentation
- Ensure all tests pass

3. **Pre-submission Checklist:**
```bash
# Format code
clang-format -i src/*.c src/*.h

# Run tests
meson test -C builddir-dev

# Check for memory leaks
meson test -C builddir-dev --wrap='valgrind --leak-check=full'

# Build documentation
# (if documentation generation is set up)
```

4. **Submit Pull Request:**
- Clear description of changes
- Reference any related issues
- Include test results
- Update CHANGELOG.md if applicable

### Release Process

1. **Version Bumping:**
```bash
# Update version in meson.build
# Update CHANGELOG.md
# Tag release
git tag -a v1.1.0 -m "Release version 1.1.0"
```

2. **Testing:**
- Full test suite on multiple distributions
- Manual testing with various PDF documents
- Performance regression testing

3. **Documentation:**
- Update README.md if needed
- Generate API documentation
- Update installation instructions

## Debugging Tips

### Common Issues

**Plugin Not Loading:**
```bash
# Check plugin path
echo $ZATHURA_PLUGIN_PATH
zathura --print-plugin-dir

# Check plugin file
ldd /path/to/libtts.so
```

**TTS Engine Issues:**
```bash
# Test engines individually
echo "test" | piper --model en_US-lessac-medium
spd-say "test"
espeak-ng "test"
```

**Memory Issues:**
```bash
# Run with AddressSanitizer
export ASAN_OPTIONS=abort_on_error=1:halt_on_error=1
./builddir-dev/tests/test-runner

# Run with Valgrind
valgrind --tool=memcheck --leak-check=full zathura document.pdf
```

### Debugging Tools

**GDB Integration:**
```bash
# Debug plugin in Zathura
gdb zathura
(gdb) set environment ZATHURA_PLUGIN_PATH=./builddir-dev/src
(gdb) run document.pdf
(gdb) break tts_plugin_init
```

**Logging:**
```c
// Add debug logging
#ifdef DEBUG
#define TTS_DEBUG(fmt, ...) \
    fprintf(stderr, "[TTS DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define TTS_DEBUG(fmt, ...)
#endif
```

This developer documentation provides a comprehensive guide for understanding and extending the Zathura TTS plugin. For additional help, consult the source code comments and test files for practical examples.