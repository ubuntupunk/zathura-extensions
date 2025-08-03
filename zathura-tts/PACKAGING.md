# Packaging Guide for Zathura TTS Plugin

This document describes the packaging system for the Zathura TTS plugin, including Debian package creation and automated build processes.

## Overview

The plugin includes comprehensive packaging support with:

- **Debian packages** for easy installation on Ubuntu/Debian systems
- **Git hooks** for automated packaging on tag pushes
- **Manual build scripts** for custom packaging
- **GitHub Actions** for CI/CD package building
- **Multi-version support** with proper dependency management

## Package Structure

### Main Packages

1. **zathura-tts** - Main plugin package
   - Plugin library (`libtts.so`)
   - Documentation files
   - Desktop integration files
   - Sample voice files (if present)

2. **zathura-tts-dev** - Development package
   - Header files for plugin development
   - Developer documentation
   - API reference materials
   - pkg-config files

3. **zathura-tts-dbgsym** - Debug symbols (automatically generated)
   - Debug symbols for troubleshooting
   - Generated automatically by debhelper

## Automated Packaging with Git Hooks

### Pre-push Hook

The pre-push hook automatically builds Debian packages when you push version tags:

```bash
# Create and push a version tag
git tag v1.0.0
git push origin v1.0.0

# The hook will automatically:
# 1. Detect the version tag
# 2. Update debian/changelog
# 3. Build source and binary packages
# 4. Create package directory with documentation
# 5. Add packages to git repository (if not in .gitignore)
```

**Hook Features:**
- Validates semantic versioning (vX.Y.Z format)
- Updates changelog automatically
- Builds both source and binary packages
- Creates comprehensive package documentation
- Organizes packages in versioned directories

### Post-commit Hook

The post-commit hook runs after each commit to:

```bash
# Automatically triggered after git commit
# 1. Builds the project in debug mode
# 2. Runs the test suite
# 3. Performs code quality checks
# 4. Creates development packages for significant commits
```

**Hook Features:**
- Only runs on main development branches
- Can be disabled with `TTS_SKIP_HOOKS=1`
- Creates development snapshots for feature commits
- Warns about TODO/FIXME comments and debug prints

### Disabling Hooks

```bash
# Temporarily disable hooks
export TTS_SKIP_HOOKS=1
git commit -m "Commit without running hooks"

# Disable for a single push
git push --no-verify origin main
```

## Manual Package Building

### Using the Build Script

The `scripts/build-deb.sh` script provides comprehensive manual packaging:

```bash
# Basic usage
./scripts/build-deb.sh 1.0.0

# Build with options
./scripts/build-deb.sh 1.0.0 --clean --test

# Source package only
./scripts/build-deb.sh 1.0.0 --source-only

# Binary package only
./scripts/build-deb.sh 1.0.0 --binary-only

# Auto-detect version from git/meson
./scripts/build-deb.sh --clean --test
```

**Script Features:**
- Automatic version detection from git tags or meson.build
- Comprehensive dependency checking
- Optional testing before packaging
- Clean build option
- Detailed package information generation
- Automatic changelog management

### Manual dpkg-buildpackage

For direct control over the build process:

```bash
# Install build dependencies
sudo apt install build-essential debhelper dh-meson meson ninja-build
sudo apt install libzathura-dev libgirara-dev libglib2.0-dev libgtk-3-dev

# Build source package
dpkg-buildpackage -S -us -uc

# Build binary package
dpkg-buildpackage -b -us -uc

# Build both
dpkg-buildpackage -us -uc
```

## GitHub Actions CI/CD

### Automated Builds

The GitHub Actions workflow (`.github/workflows/package.yml`) automatically builds packages:

**Triggers:**
- Version tag pushes (`v*`)
- Manual workflow dispatch

**Features:**
- Builds for multiple Ubuntu versions (20.04, 22.04)
- Runs comprehensive tests
- Creates release assets
- Tests package installation
- Uploads artifacts for download

### Manual Workflow Trigger

```bash
# Trigger manual build via GitHub web interface
# Go to Actions → Build Debian Packages → Run workflow
# Specify version (e.g., 1.0.0)
```

## Package Installation

### From Built Packages

```bash
# Install main package
sudo dpkg -i zathura-tts_1.0.0-1_amd64.deb
sudo apt-get install -f  # Fix dependencies

# Install with development package
sudo dpkg -i zathura-tts_1.0.0-1_amd64.deb zathura-tts-dev_1.0.0-1_amd64.deb
sudo apt-get install -f

# Install TTS engines
pip install piper  # Recommended
# OR
sudo apt install speech-dispatcher espeak-ng
```

### From Repository (Future)

```bash
# Add repository (when available)
echo "deb [trusted=yes] https://packages.zathura-tts.org/ stable main" | sudo tee /etc/apt/sources.list.d/zathura-tts.list
sudo apt update

# Install
sudo apt install zathura-tts
```

## Package Directory Structure

```
packages/
├── v1.0.0/                          # Version-specific directory
│   ├── zathura-tts_1.0.0-1_amd64.deb
│   ├── zathura-tts-dev_1.0.0-1_amd64.deb
│   ├── zathura-tts-dbgsym_1.0.0-1_amd64.deb
│   ├── zathura-tts_1.0.0-1.dsc      # Source package description
│   ├── zathura-tts_1.0.0-1.tar.xz   # Source tarball
│   ├── zathura-tts_1.0.0-1_amd64.changes
│   └── PACKAGE_INFO.md               # Installation instructions
├── dev/                              # Development snapshots
│   └── 20250802-120000-abc1234/
│       ├── zathura-tts-dev-abc1234.tar.gz
│       └── DEV_PACKAGE_INFO.md
└── ubuntu-20.04/                     # Distribution-specific builds
    └── v1.0.0/
        └── ...
```

## Debian Package Details

### Control File

The `debian/control` file defines:
- Package metadata and descriptions
- Build dependencies
- Runtime dependencies
- Package relationships
- Standards compliance

### Build Configuration

- **Build System**: Meson with debhelper integration
- **Compatibility Level**: 13 (modern debhelper)
- **Source Format**: 3.0 (quilt) for patch management
- **Hardening**: All security hardening options enabled

### Installation Scripts

- **postinst**: Updates plugin cache, shows setup instructions
- **prerm**: Cleans up plugin cache on removal
- **install files**: Define what files go in each package

## Troubleshooting Packaging

### Common Build Issues

**Missing Dependencies:**
```bash
# Install all build dependencies
sudo apt build-dep .
# OR manually install from debian/control
```

**Version Conflicts:**
```bash
# Check version consistency
grep version meson.build
git describe --tags
head -1 debian/changelog
```

**Build Failures:**
```bash
# Clean and retry
./scripts/build-deb.sh --clean --test
```

### Testing Packages

**Installation Test:**
```bash
# Test in clean environment (Docker)
docker run -it ubuntu:22.04
apt update && apt install -y ./zathura-tts_*.deb
zathura --list-plugins | grep tts
```

**Functionality Test:**
```bash
# Install TTS engine
sudo apt install espeak-ng

# Test basic functionality
zathura test-document.pdf
# Press Ctrl+T to test TTS
```

### Package Quality Checks

**Lintian Analysis:**
```bash
# Check package quality
lintian zathura-tts_1.0.0-1_amd64.deb

# Check source package
lintian zathura-tts_1.0.0-1.dsc
```

**Dependency Analysis:**
```bash
# Check dependencies
dpkg-deb --info zathura-tts_1.0.0-1_amd64.deb
apt-cache depends zathura-tts
```

## Contributing to Packaging

### Adding New Package Types

1. Create new package definition in `debian/control`
2. Add corresponding `.install` file
3. Update build scripts if needed
4. Test package creation and installation

### Improving Build Process

1. Modify `debian/rules` for build customization
2. Update `scripts/build-deb.sh` for new features
3. Enhance GitHub Actions workflow
4. Update documentation

### Platform Support

To add support for new distributions:

1. Create distribution-specific control files
2. Add build matrix entries in GitHub Actions
3. Test package compatibility
4. Update installation documentation

## Release Process

### Version Release Workflow

1. **Prepare Release:**
   ```bash
   # Update version in meson.build
   # Update documentation
   # Run full test suite
   meson test -C builddir
   ```

2. **Create Release:**
   ```bash
   # Commit changes
   git add .
   git commit -m "Prepare release v1.0.0"
   
   # Create and push tag
   git tag v1.0.0
   git push origin main
   git push origin v1.0.0
   ```

3. **Automated Process:**
   - Git hooks build packages automatically
   - GitHub Actions creates release assets
   - Packages are uploaded to GitHub releases

4. **Post-Release:**
   - Test package installation
   - Update distribution repositories
   - Announce release

This comprehensive packaging system ensures reliable, automated package creation and distribution for the Zathura TTS plugin across multiple platforms and use cases.