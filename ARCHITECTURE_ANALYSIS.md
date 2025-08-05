# Architecture Analysis: Plugin vs Application Repository Structure

## Current Issue: Architectural Inversion

We currently have an inverted architecture where the plugin repository contains the main application as a submodule:

```
zathura-liberated/ (plugin repo)
├── zathura-tts/ (plugin code)
└── zathura/ (submodule - main application)
```

This is backwards from normal plugin development patterns.

## Standard Plugin Architecture

Normal plugin development follows this pattern:

```
zathura/ (main application repo)
├── core application code
└── plugin API headers

zathura-plugin-pdf/ (separate plugin repo)
├── plugin implementation
└── depends on: zathura-dev

zathura-plugin-tts/ (our plugin repo)
├── plugin implementation  
└── depends on: zathura-dev (with utility plugin support)
```

## Why We Have This Inversion

1. **Core Modification Required**: We needed to add utility plugin support to Zathura core
2. **API Extension**: Added `zathura_get_session()` and enhanced plugin loading
3. **Development Convenience**: Having everything in one repo made development easier
4. **Submodule Approach**: We used submodules to manage the modified Zathura

## Potential Solutions

### Option 1: Split Into Two Repositories (Recommended)

**Repository 1: zathura-with-utility-plugins**
```
zathura-with-utility-plugins/
├── zathura/ (fork with our modifications)
├── README.md (explains utility plugin support)
├── patches/ (for upstream contribution)
└── build instructions
```

**Repository 2: zathura-tts-plugin**
```
zathura-tts-plugin/
├── src/ (plugin code)
├── depends on: zathura-with-utility-plugins
├── meson.build
└── README.md
```

**Benefits:**
- ✅ Follows standard plugin architecture
- ✅ Clear separation of concerns
- ✅ Plugin can be used with any Zathura that has utility plugin support
- ✅ Easier to maintain and understand

**Drawbacks:**
- ⚠️ Users need to install two components
- ⚠️ More complex setup process

### Option 2: Monorepo with Clear Structure

**Current repo restructured:**
```
zathura-liberated/
├── zathura-core/ (our modified Zathura)
├── plugins/
│   └── zathura-tts/ (plugin code)
├── docs/
├── scripts/ (build automation)
└── README.md (explains the monorepo approach)
```

**Benefits:**
- ✅ Everything in one place
- ✅ Simplified build process
- ✅ Clear project structure

**Drawbacks:**
- ⚠️ Still architecturally unusual
- ⚠️ Harder to contribute plugin separately

### Option 3: Upstream-First Approach

**Steps:**
1. Submit our Zathura modifications as PR to upstream
2. Wait for acceptance (or maintain fork)
3. Create separate plugin repository that depends on upstream Zathura

**Benefits:**
- ✅ Follows standard architecture perfectly
- ✅ Benefits entire Zathura ecosystem
- ✅ Plugin becomes a standard Zathura plugin

**Drawbacks:**
- ⚠️ Depends on upstream acceptance
- ⚠️ May take significant time
- ⚠️ Upstream may reject utility plugin concept

### Option 4: Hybrid Approach (Current + Cleanup)

**Keep current structure but improve it:**
```
zathura-liberated/
├── README.md (explains why we have this structure)
├── zathura/ (submodule - clearly marked as modified core)
├── zathura-tts/ (plugin)
├── docs/
│   ├── WHY_MONOREPO.md
│   └── ARCHITECTURE_DECISIONS.md
└── scripts/build-all.sh
```

**Benefits:**
- ✅ Minimal disruption to current working setup
- ✅ Clear documentation of architectural decisions
- ✅ Maintains development velocity

## Recommendation: Option 1 (Split Repositories)

Given that we now have a working system, I recommend **Option 1** for these reasons:

### Technical Benefits
- **Standard Architecture**: Follows established plugin development patterns
- **Modularity**: Each component has a clear, single responsibility
- **Maintainability**: Easier to maintain and understand each piece
- **Reusability**: Plugin can work with any Zathura that has utility plugin support

### User Benefits
- **Clear Dependencies**: Users understand what they're installing
- **Flexibility**: Can use different versions of core vs plugin
- **Standard Expectations**: Matches how other Zathura plugins work

### Developer Benefits
- **Separation of Concerns**: Core modifications vs plugin development
- **Easier Contribution**: Can contribute to upstream Zathura separately
- **Standard Workflow**: Follows established open-source patterns

## Implementation Plan for Option 1

### Phase 1: Create Core Repository
```bash
# Create new repo: zathura-with-utility-plugins
git clone https://github.com/ubuntupunk/zathura.git zathura-with-utility-plugins
cd zathura-with-utility-plugins
git checkout feature/utility-plugin-support
# Add documentation, build scripts, etc.
```

### Phase 2: Create Plugin Repository
```bash
# Create new repo: zathura-tts-plugin
mkdir zathura-tts-plugin
cd zathura-tts-plugin
# Copy plugin code from current zathura-tts/
# Update build system to depend on zathura-with-utility-plugins
```

### Phase 3: Update Documentation
- Update README files to explain the new structure
- Create installation guides for both components
- Document the relationship between the repositories

### Phase 4: Migration Guide
- Provide clear migration instructions for existing users
- Maintain current repo with deprecation notice and migration guide
- Update all documentation to point to new repositories

## Conclusion

While our current inverted architecture worked well for development, splitting into two repositories would:
1. Follow standard plugin architecture patterns
2. Make the project more maintainable and understandable
3. Align with user expectations for plugin development
4. Facilitate potential upstream contribution

The working system we have proves the concept works - now we can restructure it properly.