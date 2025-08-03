#!/bin/bash
#
# Manual Debian package builder for Zathura TTS plugin
# Usage: ./scripts/build-deb.sh [version]
#

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    echo -e "${BLUE}[BUILD]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Show usage
show_usage() {
    echo "Usage: $0 [version] [options]"
    echo ""
    echo "Options:"
    echo "  -h, --help          Show this help message"
    echo "  -s, --source-only   Build source package only"
    echo "  -b, --binary-only   Build binary package only"
    echo "  -c, --clean         Clean build artifacts before building"
    echo "  -t, --test          Run tests before building"
    echo "  --no-sign          Don't sign packages"
    echo ""
    echo "Examples:"
    echo "  $0 1.0.0                    # Build packages for version 1.0.0"
    echo "  $0 1.0.0 --source-only      # Build source package only"
    echo "  $0 --clean --test           # Clean, test, and build current version"
    echo ""
    echo "If no version is specified, it will be extracted from git tags or meson.build"
}

# Parse command line arguments
VERSION=""
SOURCE_ONLY=false
BINARY_ONLY=false
CLEAN=false
RUN_TESTS=false
NO_SIGN=false

while [[ $# -gt 0 ]]; do
    case $1 in
        -h|--help)
            show_usage
            exit 0
            ;;
        -s|--source-only)
            SOURCE_ONLY=true
            shift
            ;;
        -b|--binary-only)
            BINARY_ONLY=true
            shift
            ;;
        -c|--clean)
            CLEAN=true
            shift
            ;;
        -t|--test)
            RUN_TESTS=true
            shift
            ;;
        --no-sign)
            NO_SIGN=true
            shift
            ;;
        -*)
            print_error "Unknown option: $1"
            show_usage
            exit 1
            ;;
        *)
            if [ -z "$VERSION" ]; then
                VERSION="$1"
            else
                print_error "Multiple versions specified"
                exit 1
            fi
            shift
            ;;
    esac
done

# Check if we're in the right directory
if [ ! -f "meson.build" ] || [ ! -d "debian" ]; then
    print_error "This script must be run from the project root directory"
    print_error "Make sure you have meson.build and debian/ directory"
    exit 1
fi

# Determine version if not specified
if [ -z "$VERSION" ]; then
    # Try to get version from git tag
    if git describe --tags --exact-match HEAD >/dev/null 2>&1; then
        VERSION=$(git describe --tags --exact-match HEAD | sed 's/^v//')
        print_status "Using version from git tag: $VERSION"
    else
        # Try to get version from meson.build
        if grep -q "version.*:" meson.build; then
            VERSION=$(grep "version.*:" meson.build | sed "s/.*version.*:[[:space:]]*['\"]\\([^'\"]*\\)['\"].*/\\1/")
            print_status "Using version from meson.build: $VERSION"
        else
            print_error "Could not determine version. Please specify version as argument."
            exit 1
        fi
    fi
fi

print_status "Building Debian packages for Zathura TTS Plugin v$VERSION"

# Validate version format
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    print_warning "Version '$VERSION' doesn't follow semantic versioning (X.Y.Z)"
    read -p "Continue anyway? (y/N): " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]; then
        exit 1
    fi
fi

# Check dependencies
print_status "Checking build dependencies..."

MISSING_DEPS=()

if ! command -v dpkg-buildpackage >/dev/null 2>&1; then
    MISSING_DEPS+=("dpkg-dev")
fi

if ! command -v dh >/dev/null 2>&1; then
    MISSING_DEPS+=("debhelper")
fi

if ! command -v meson >/dev/null 2>&1; then
    MISSING_DEPS+=("meson")
fi

if [ ${#MISSING_DEPS[@]} -gt 0 ]; then
    print_error "Missing build dependencies: ${MISSING_DEPS[*]}"
    print_status "Install with: sudo apt install ${MISSING_DEPS[*]}"
    exit 1
fi

# Clean if requested
if [ "$CLEAN" = true ]; then
    print_status "Cleaning build artifacts..."
    rm -rf builddir builddir-* debian/.debhelper debian/tmp
    rm -f debian/files debian/*.substvars debian/*.debhelper.log
    find .. -maxdepth 1 -name "zathura-tts_*" -delete 2>/dev/null || true
    print_success "Clean completed"
fi

# Run tests if requested
if [ "$RUN_TESTS" = true ]; then
    print_status "Running test suite..."
    
    if [ ! -d "builddir-test" ]; then
        meson setup builddir-test --buildtype=debug -Dtests=true
    fi
    
    meson compile -C builddir-test
    
    if meson test -C builddir-test; then
        print_success "All tests passed"
    else
        print_error "Tests failed"
        exit 1
    fi
fi

# Update changelog
print_status "Updating debian/changelog..."

# Backup original changelog
cp debian/changelog debian/changelog.backup

# Create new changelog entry
temp_changelog=$(mktemp)
cat > "$temp_changelog" << EOF
zathura-tts ($VERSION-1) unstable; urgency=medium

  * Release version $VERSION
  * $(git log --oneline -1 --pretty=format:"%s" 2>/dev/null || echo "Manual package build")

 -- Zathura TTS Development Team <dev@zathura-tts.org>  $(date -R)

EOF

# Append existing changelog
cat debian/changelog >> "$temp_changelog"
mv "$temp_changelog" debian/changelog

print_success "Changelog updated"

# Prepare build options
BUILD_OPTIONS=""
if [ "$NO_SIGN" = true ]; then
    BUILD_OPTIONS="$BUILD_OPTIONS -us -uc"
fi

# Build packages
if [ "$SOURCE_ONLY" = true ]; then
    print_status "Building source package only..."
    if dpkg-buildpackage -S $BUILD_OPTIONS; then
        print_success "Source package built successfully"
    else
        print_error "Source package build failed"
        exit 1
    fi
elif [ "$BINARY_ONLY" = true ]; then
    print_status "Building binary package only..."
    if dpkg-buildpackage -b $BUILD_OPTIONS; then
        print_success "Binary package built successfully"
    else
        print_error "Binary package build failed"
        exit 1
    fi
else
    print_status "Building source and binary packages..."
    if dpkg-buildpackage $BUILD_OPTIONS; then
        print_success "All packages built successfully"
    else
        print_error "Package build failed"
        exit 1
    fi
fi

# Create package directory and move files
PACKAGE_DIR="packages/v$VERSION"
mkdir -p "$PACKAGE_DIR"

print_status "Moving packages to $PACKAGE_DIR..."

# Move all generated package files
mv ../zathura-tts_${VERSION}-1* "$PACKAGE_DIR/" 2>/dev/null || true
mv ../zathura-tts-dev_${VERSION}-1* "$PACKAGE_DIR/" 2>/dev/null || true
mv ../zathura-tts-dbgsym_${VERSION}-1* "$PACKAGE_DIR/" 2>/dev/null || true

# Create package information file
cat > "$PACKAGE_DIR/PACKAGE_INFO.md" << EOF
# Zathura TTS Plugin v$VERSION - Debian Packages

## Package Information

- **Version**: $VERSION
- **Build Date**: $(date)
- **Git Commit**: $(git rev-parse HEAD 2>/dev/null || echo "Unknown")
- **Builder**: $(whoami)@$(hostname)

## Installation

### Quick Install:
\`\`\`bash
sudo dpkg -i zathura-tts_${VERSION}-1_*.deb
sudo apt-get install -f  # Fix any dependency issues
\`\`\`

### With Development Package:
\`\`\`bash
sudo dpkg -i zathura-tts_${VERSION}-1_*.deb zathura-tts-dev_${VERSION}-1_*.deb
sudo apt-get install -f
\`\`\`

## TTS Engine Setup

After installation, install at least one TTS engine:

\`\`\`bash
# High-quality neural voices (recommended)
pip install piper

# System TTS integration
sudo apt install speech-dispatcher

# Lightweight fallback
sudo apt install espeak-ng
\`\`\`

## Quick Start

1. Open a PDF: \`zathura document.pdf\`
2. Start reading: Press \`Ctrl+T\`
3. Pause/Resume: Press \`Space\`
4. Settings: Press \`Ctrl+Shift+T\`

## Documentation

- Quick Start: \`/usr/share/doc/zathura-tts/QUICKSTART.md\`
- Installation: \`/usr/share/doc/zathura-tts/INSTALLATION.md\`
- Troubleshooting: \`/usr/share/doc/zathura-tts/TROUBLESHOOTING.md\`
- Developer Guide: \`/usr/share/doc/zathura-tts/DEVELOPER.md\`

## Package Contents

EOF

# List package files
for pkg in "$PACKAGE_DIR"/*.deb; do
    if [ -f "$pkg" ]; then
        echo "### $(basename "$pkg")" >> "$PACKAGE_DIR/PACKAGE_INFO.md"
        dpkg-deb --info "$pkg" | grep "^ Description:" >> "$PACKAGE_DIR/PACKAGE_INFO.md"
        echo "" >> "$PACKAGE_DIR/PACKAGE_INFO.md"
    fi
done

# Restore original changelog
mv debian/changelog.backup debian/changelog

print_success "Debian packaging completed!"
echo ""
print_status "Package files created:"
ls -la "$PACKAGE_DIR"
echo ""
print_status "To install:"
echo "  sudo dpkg -i $PACKAGE_DIR/zathura-tts_${VERSION}-1_*.deb"
echo "  sudo apt-get install -f"
echo ""
print_status "Package information: $PACKAGE_DIR/PACKAGE_INFO.md"