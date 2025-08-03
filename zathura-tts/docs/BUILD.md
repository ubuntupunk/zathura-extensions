# Build and Development Setup

This guide provides detailed instructions for setting up a development environment and building the Zathura TTS plugin from source.

## Table of Contents

1. [Development Environment Setup](#development-environment-setup)
2. [Build System Overview](#build-system-overview)
3. [Building the Plugin](#building-the-plugin)
4. [Development Workflow](#development-workflow)
5. [Testing](#testing)
6. [Debugging](#debugging)
7. [Code Quality](#code-quality)
8. [Packaging](#packaging)

## Development Environment Setup

### Prerequisites

**System Requirements:**
- Linux-based operating system (Ubuntu 20.04+, Fedora 35+, Arch Linux, etc.)
- GCC 9.0+ or Clang 10.0+
- Meson 0.56.0+
- Ninja build system
- pkg-config
- Git

**Development Libraries:**
```bash
# Ubuntu/Debian
sudo apt install build-essential meson ninja-build pkg-config git
sudo apt install zathura-dev libgirara-dev libglib2.0-dev libgtk-3-dev
sudo apt install libcairo2-dev libpoppler-glib-dev

# Fedora/RHEL
sudo dnf install gcc meson ninja-build pkgconfig git
sudo dnf install zathura-devel girara-devel glib2-devel gtk3-devel
sudo dnf install cairo-devel poppler-glib-devel

# Arch Linux
sudo pacman -S base-devel meson ninja pkgconf git
sudo pacman -S zathura girara glib2 gtk3 cairo poppler-glib
```

**Development Tools (Optional but Recommended):**
```bash
# Debugging and analysis tools
sudo apt install gdb valgrind clang-tools clang-format
sudo apt install doxygen graphviz  # For documentation generation

# TTS engines for testing
pip install piper
sudo apt install speech-dispatcher espeak-ng
```

### IDE/Editor Setup

**Visual Studio Code:**
```bash
# Install VS Code extensions
code --install-extension ms-vscode.cpptools
code --install-extension ms-vscode.cmake-tools
code --install-extension twxs.cmake

# Create workspace settings
mkdir -p .vscode
cat > .vscode/settings.json << 'EOF'
{
    "C_Cpp.default.configurationProvider": "ms-vscode.cmake-tools",
    "C_Cpp.default.compileCommands": "${workspaceFolder}/builddir/compile_commands.json",
    "C_Cpp.default.cStandard": "c11",
    "files.associations": {
        "*.h": "c",
        "*.c": "c"
    },
    "editor.tabSize": 4,
    "editor.insertSpaces": true,
    "C_Cpp.clang_format_style": "file"
}
EOF
```

**Vim/Neovim:**
```vim
" Add to .vimrc/.init.vim
set path+=src/,tests/,docs/
set tags=./tags,tags
set expandtab
set tabstop=4
set shiftwidth=4

" For C development
autocmd FileType c setlocal cindent
autocmd FileType c setlocal formatoptions-=cro
```

**Emacs:**
```elisp
;; Add to .emacs or init.el
(setq c-default-style "linux"
      c-basic-offset 4)
(add-hook 'c-mode-hook
          (lambda ()
            (setq indent-tabs-mode nil)
            (setq tab-width 4)))
```

## Build System Overview

The project uses the Meson build system with Ninja as the backend. The build configuration is defined in `meson.build` files.

### Project Structure
```
zathura-tts/
├── meson.build              # Main build configuration
├── meson_options.txt        # Build options
├── src/
│   └── meson.build         # Source build configuration
├── tests/
│   └── meson.build         # Test build configuration
├── data/
│   └── meson.build         # Data files configuration
└── docs/
    └── meson.build         # Documentation configuration
```

### Build Options

Available build options (defined in `meson_options.txt`):

```meson
# Debug options
option('debug', type: 'boolean', value: false, description: 'Enable debug build')
option('optimization', type: 'combo', choices: ['0', '1', '2', '3', 's'], value: '2')

# Feature options
option('tests', type: 'boolean', value: true, description: 'Build test suite')
option('docs', type: 'boolean', value: false, description: 'Build documentation')
option('examples', type: 'boolean', value: false, description: 'Build examples')

# TTS engine options
option('piper_support', type: 'boolean', value: true, description: 'Enable Piper-TTS support')
option('speechd_support', type: 'boolean', value: true, description: 'Enable Speech Dispatcher support')
option('espeak_support', type: 'boolean', value: true, description: 'Enable espeak-ng support')

# Installation options
option('plugin_dir', type: 'string', value: '', description: 'Plugin installation directory')
```

## Building the Plugin

### Quick Build

```bash
# Clone repository
git clone https://github.com/zathura-pdf/zathura-tts.git
cd zathura-tts

# Configure and build
meson setup builddir
meson compile -C builddir

# Install
sudo meson install -C builddir
```

### Development Build

```bash
# Debug build with all features enabled
meson setup builddir-dev \
    --buildtype=debug \
    --optimization=0 \
    -Dtests=true \
    -Ddocs=true \
    -Dwarning_level=3

# Compile with verbose output
meson compile -C builddir-dev -v

# Install to local directory for testing
meson install -C builddir-dev --destdir=./install-test
```

### Cross-Compilation

```bash
# Example: Cross-compile for ARM64
meson setup builddir-arm64 --cross-file=cross/aarch64-linux-gnu.txt
meson compile -C builddir-arm64
```

### Build Configuration Examples

**Release Build:**
```bash
meson setup builddir-release \
    --buildtype=release \
    --optimization=3 \
    --strip \
    -Dtests=false \
    -Ddocs=false
```

**Debug Build with Sanitizers:**
```bash
meson setup builddir-debug \
    --buildtype=debug \
    -Db_sanitize=address,undefined \
    -Db_coverage=true \
    -Dtests=true
```

**Minimal Build:**
```bash
meson setup builddir-minimal \
    --buildtype=minsize \
    -Dpiper_support=false \
    -Dspeechd_support=false \
    -Dtests=false
```

## Development Workflow

### 1. Setting Up Development Environment

```bash
# Fork and clone repository
git clone https://github.com/yourusername/zathura-tts.git
cd zathura-tts

# Set up upstream remote
git remote add upstream https://github.com/zathura-pdf/zathura-tts.git

# Create development build
meson setup builddir-dev --buildtype=debug -Dtests=true
```

### 2. Making Changes

```bash
# Create feature branch
git checkout -b feature/my-new-feature

# Make changes to source code
# Edit src/*.c, src/*.h files

# Build and test changes
meson compile -C builddir-dev
meson test -C builddir-dev
```

### 3. Code Formatting

```bash
# Format code using clang-format
find src tests -name "*.c" -o -name "*.h" | xargs clang-format -i

# Check formatting
find src tests -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

### 4. Testing Changes

```bash
# Run all tests
meson test -C builddir-dev

# Run specific test
meson test -C builddir-dev unit-tests

# Run tests with verbose output
meson test -C builddir-dev --verbose

# Test with real Zathura instance
export ZATHURA_PLUGIN_PATH=./builddir-dev/src
zathura test-document.pdf
```

### 5. Committing Changes

```bash
# Stage changes
git add .

# Commit with descriptive message
git commit -m "Add new TTS engine support

- Implement MyTTS engine interface
- Add configuration options for MyTTS
- Update documentation
- Add unit tests for new functionality"

# Push to your fork
git push origin feature/my-new-feature
```

## Testing

### Test Suite Structure

```
tests/
├── test-framework.c         # Testing framework implementation
├── test-framework.h         # Testing framework headers
├── test-simple.c           # Basic unit tests
├── test-integration-simple.c # Integration tests
├── test-audio-controller.c # Audio controller tests
├── test-main.c             # Test runner
└── meson.build             # Test build configuration
```

### Running Tests

**All Tests:**
```bash
meson test -C builddir-dev
```

**Specific Test Suites:**
```bash
# Unit tests only
meson test -C builddir-dev unit-tests

# Integration tests only
meson test -C builddir-dev integration-tests

# Specific test file
meson test -C builddir-dev test-audio-controller
```

**Test Options:**
```bash
# Verbose output
meson test -C builddir-dev --verbose

# Run tests multiple times
meson test -C builddir-dev --repeat 10

# Run tests in parallel
meson test -C builddir-dev --num-processes 4

# Run tests with timeout
meson test -C builddir-dev --timeout-multiplier 2
```

### Memory Testing

**Valgrind:**
```bash
# Run tests under Valgrind
meson test -C builddir-dev --wrap='valgrind --leak-check=full --show-leak-kinds=all'

# Specific test with Valgrind
meson test -C builddir-dev test-audio-controller --wrap='valgrind --leak-check=full'
```

**AddressSanitizer:**
```bash
# Configure build with AddressSanitizer
meson configure builddir-dev -Db_sanitize=address

# Rebuild and test
meson compile -C builddir-dev
meson test -C builddir-dev
```

### Coverage Analysis

```bash
# Enable coverage
meson configure builddir-dev -Db_coverage=true

# Rebuild and run tests
meson compile -C builddir-dev
meson test -C builddir-dev

# Generate coverage report
ninja -C builddir-dev coverage-html

# View coverage report
xdg-open builddir-dev/meson-logs/coveragereport/index.html
```

## Debugging

### GDB Integration

**Debug Plugin in Zathura:**
```bash
# Set up environment
export ZATHURA_PLUGIN_PATH=./builddir-dev/src

# Start GDB
gdb zathura

# GDB commands
(gdb) set environment ZATHURA_PLUGIN_PATH=./builddir-dev/src
(gdb) break tts_plugin_init
(gdb) run test-document.pdf
(gdb) continue
```

**Debug Tests:**
```bash
# Debug specific test
gdb ./builddir-dev/tests/test-audio-controller-runner

# Set breakpoints and run
(gdb) break test_audio_controller_creation
(gdb) run
```

### Debug Builds

**Enable Debug Symbols:**
```bash
meson configure builddir-dev -Ddebug=true -Doptimization=0
meson compile -C builddir-dev
```

**Debug Macros:**
```c
// Add to source files for debug output
#ifdef DEBUG
#define TTS_DEBUG(fmt, ...) \
    fprintf(stderr, "[TTS DEBUG] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define TTS_DEBUG(fmt, ...)
#endif

// Usage
TTS_DEBUG("Audio controller state: %d", controller->state);
```

### Logging

**Enable Runtime Logging:**
```bash
# Set environment variables
export TTS_DEBUG=1
export TTS_LOG_LEVEL=debug
export TTS_LOG_FILE=/tmp/tts-debug.log

# Run Zathura
zathura document.pdf

# Monitor logs
tail -f /tmp/tts-debug.log
```

## Code Quality

### Static Analysis

**Clang Static Analyzer:**
```bash
# Run static analysis
scan-build meson compile -C builddir-dev

# View results
scan-view /tmp/scan-build-*
```

**Cppcheck:**
```bash
# Install cppcheck
sudo apt install cppcheck

# Run analysis
cppcheck --enable=all --inconclusive --std=c11 src/
```

### Code Formatting

**clang-format Configuration (`.clang-format`):**
```yaml
BasedOnStyle: LLVM
IndentWidth: 4
UseTab: Never
ColumnLimit: 100
BreakBeforeBraces: Linux
AllowShortFunctionsOnASingleLine: None
AlignConsecutiveDeclarations: true
AlignConsecutiveAssignments: true
```

**Format Code:**
```bash
# Format all source files
find src tests -name "*.c" -o -name "*.h" | xargs clang-format -i

# Check formatting without modifying files
find src tests -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror
```

### Linting

**Custom Lint Script (`scripts/lint.sh`):**
```bash
#!/bin/bash
set -e

echo "Running code quality checks..."

# Check formatting
echo "Checking code formatting..."
find src tests -name "*.c" -o -name "*.h" | xargs clang-format --dry-run --Werror

# Run static analysis
echo "Running static analysis..."
cppcheck --enable=all --error-exitcode=1 --quiet src/

# Check for common issues
echo "Checking for common issues..."
grep -r "TODO\|FIXME\|XXX" src/ && echo "Found TODO/FIXME comments" || true

echo "All checks passed!"
```

## Packaging

### Creating Distribution Packages

**Source Tarball:**
```bash
# Create release tarball
meson dist -C builddir-dev

# Tarball will be created in builddir-dev/meson-dist/
```

**Debian Package:**
```bash
# Install packaging tools
sudo apt install debhelper dh-meson

# Create debian/ directory with packaging files
# Build package
dpkg-buildpackage -us -uc
```

**RPM Package:**
```bash
# Create RPM spec file
# Build with rpmbuild
rpmbuild -ba zathura-tts.spec
```

### Installation Verification

**Test Installation:**
```bash
# Install to temporary directory
DESTDIR=/tmp/test-install meson install -C builddir-dev

# Verify files are installed correctly
find /tmp/test-install -type f -name "*.so"
find /tmp/test-install -type f -name "*.desktop"

# Test plugin loading
export ZATHURA_PLUGIN_PATH=/tmp/test-install/usr/local/lib/zathura
zathura --list-plugins | grep tts
```

## Continuous Integration

### GitHub Actions Workflow (`.github/workflows/ci.yml`)

```yaml
name: CI

on: [push, pull_request]

jobs:
  build-and-test:
    runs-on: ubuntu-latest
    strategy:
      matrix:
        compiler: [gcc, clang]
        buildtype: [debug, release]
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install dependencies
      run: |
        sudo apt update
        sudo apt install -y meson ninja-build gcc clang
        sudo apt install -y zathura-dev libgirara-dev libglib2.0-dev
        pip install piper
    
    - name: Configure
      run: |
        CC=${{ matrix.compiler }} meson setup builddir \
          --buildtype=${{ matrix.buildtype }} \
          -Dtests=true
    
    - name: Build
      run: meson compile -C builddir
    
    - name: Test
      run: meson test -C builddir --verbose
    
    - name: Upload test results
      uses: actions/upload-artifact@v3
      if: failure()
      with:
        name: test-results-${{ matrix.compiler }}-${{ matrix.buildtype }}
        path: builddir/meson-logs/testlog.txt
```

This comprehensive build and development guide provides everything needed to set up a development environment, build the plugin, and maintain code quality throughout the development process.