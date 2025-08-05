# Submodule Setup and Fork Management

## Overview
This document explains how we've set up the Zathura submodule to use our fork with the utility plugin modifications.

## Current Configuration

### Main Repository
- **Repository**: `https://github.com/ubuntupunk/zathura-liberated.git`
- **Branch**: `main`
- **Contains**: TTS plugin code, documentation, build configuration

### Zathura Submodule (Fork)
- **Repository**: `git@github.com-zathura:ubuntupunk/zathura.git`
- **Branch**: `feature/utility-plugin-support`
- **Contains**: Modified Zathura with utility plugin architecture
- **Commit**: `9ebd3a0149306665f4db151f4688ba3431dd9a3f`

### PDF Plugin Submodule (Upstream)
- **Repository**: `https://github.com/pwmt/zathura-pdf-poppler.git`
- **Branch**: `develop`
- **Status**: Unmodified upstream version

## What We've Done

### 1. ✅ Created Proper Fork Structure
```bash
# Original upstream
https://github.com/pwmt/zathura.git

# Our fork
https://github.com/ubuntupunk/zathura.git
├── develop (upstream tracking)
└── feature/utility-plugin-support (our modifications)
```

### 2. ✅ Updated Submodule Configuration
```ini
# .gitmodules
[submodule "zathura"]
    path = zathura
    url = git@github.com-zathura:ubuntupunk/zathura.git
```

### 3. ✅ Proper Branch Management
- **Feature Branch**: `feature/utility-plugin-support` contains our modifications
- **Clean History**: All changes are in a single, well-documented commit
- **PR Ready**: Branch is ready for upstream pull request when appropriate

## Benefits of This Approach

### ✅ **Professional Development Workflow**
- Follows Git best practices with feature branches
- Maintains clean separation between upstream and our changes
- Ready for pull request submission to upstream

### ✅ **User-Friendly Installation**
- Users can clone with `--recursive` and get everything they need
- Submodule automatically points to our fork with modifications
- No manual patch application required

### ✅ **Upstream Integration Ready**
- Changes are isolated in a feature branch
- Clean commit history makes review easier
- Can easily create PR: `feature/utility-plugin-support` → `upstream/develop`

### ✅ **Maintenance Friendly**
- Can easily sync with upstream changes
- Can rebase our feature branch on latest upstream
- Clear separation between our code and upstream code

## For Users

### Clone and Build
```bash
# Get everything including our modified Zathura
git clone --recursive https://github.com/ubuntupunk/zathura-liberated.git
cd zathura-liberated

# Build modified Zathura (automatically uses our fork)
cd zathura
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir

# Build TTS plugin
cd ../zathura-tts
meson setup builddir
meson compile -C builddir
sudo meson install -C builddir
```

### Alternative: Use Fork Directly
```bash
# Clone our fork directly
git clone https://github.com/ubuntupunk/zathura.git
cd zathura
git checkout feature/utility-plugin-support
# Build and install
```

## For Developers

### Sync with Upstream
```bash
cd zathura
git remote add upstream https://github.com/pwmt/zathura.git
git fetch upstream
git checkout develop
git merge upstream/develop
git push origin develop

# Rebase our feature branch if needed
git checkout feature/utility-plugin-support
git rebase develop
```

### Create Upstream PR
When ready to contribute back to upstream:
1. Push feature branch to our fork (already done)
2. Create PR: `ubuntupunk/zathura:feature/utility-plugin-support` → `pwmt/zathura:develop`
3. Include our comprehensive documentation and testing results

## Current Status

### ✅ **Fully Functional**
- TTS plugin loads successfully with our modified Zathura
- All build processes work correctly
- Documentation is comprehensive and up-to-date

### ✅ **Ready for Distribution**
- Users can clone and build everything they need
- Clear installation instructions provided
- Compatibility notes documented

### ✅ **Ready for Upstream Contribution**
- Clean feature branch with well-documented changes
- Comprehensive testing and documentation
- Professional development workflow followed

## Compatibility Considerations

### GLib Versions
- **Tested**: GLib 2.74.6 (Debian 12)
- **Minimum**: GLib 2.50+ (as per Zathura requirements)
- **Note**: May need testing/adjustments for newer GLib versions

### Upstream Sync
- Our fork is based on Zathura commit `65dedaa` (0.5.12)
- May need periodic syncing with upstream development
- Feature branch can be rebased on newer upstream versions as needed

This setup provides a solid foundation for both user adoption and potential upstream contribution.