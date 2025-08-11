# TTS Implementation Research: PDF Readers and Document Viewers

## Current Issues to Solve
1. **Lengthy delays between reading chunks** - Need smoother transitions
2. **Multiple/overlapping readers** - Better process management needed
3. **Audio continuity** - Seamless reading experience

## Research Targets

### 1. Adobe Acrobat Reader TTS
- **Method**: Uses system TTS APIs directly (SAPI on Windows, Speech Synthesis on macOS)
- **Architecture**: Integrates with OS speech services rather than spawning external processes
- **Advantages**: 
  - No process spawning overhead
  - Native pause/resume/speed controls
  - Seamless audio streaming
  - Better resource management

### 2. NVDA Screen Reader (PDF support)
- **Method**: Uses speech synthesis APIs with buffering
- **Architecture**: 
  - Text preprocessing and chunking
  - Audio buffering for smooth playback
  - Queue-based speech management
- **Key Features**:
  - Intelligent text segmentation
  - Audio stream continuity
  - Interrupt and resume capabilities

### 3. Okular (KDE PDF Viewer)
- **Method**: Uses Qt Speech module (QTextToSpeech)
- **Architecture**: 
  - Qt's speech synthesis framework
  - Event-driven speech management
  - Built-in audio controls
- **Advantages**:
  - No external process spawning
  - Native Qt integration
  - Smooth audio transitions

### 4. Evince (GNOME PDF Viewer)
- **Method**: Uses Speech Dispatcher or AT-SPI
- **Architecture**:
  - System accessibility services
  - Asynchronous speech requests
  - Integrated with GNOME accessibility

## Key Insights for Our Implementation

### Problem 1: Process Spawning Overhead
**Current Issue**: We spawn new processes for each text segment
**Better Approach**: 
- Use persistent TTS service/daemon
- Implement audio streaming instead of discrete processes
- Consider using Speech Dispatcher's streaming API

### Problem 2: Audio Gaps Between Segments
**Current Issue**: Delays between finishing one segment and starting next
**Better Approach**:
- Pre-buffer next segment while current is playing
- Use audio streaming with continuous playback
- Implement audio queue management

### Problem 3: Process Management Complexity
**Current Issue**: Complex process lifecycle with kill/wait cycles
**Better Approach**:
- Use library-based TTS instead of process spawning
- Implement single persistent TTS service
- Use callback-based audio completion detection

## Recommended Architecture Changes

### Option A: Speech Dispatcher Integration (Recommended)
- Use Speech Dispatcher's streaming API
- Single persistent connection
- Queue-based text management
- Native pause/resume/speed controls

### Option B: Direct TTS Library Integration
- Link against TTS libraries directly (espeak-ng lib, piper lib)
- Eliminate process spawning entirely
- Implement audio streaming pipeline

### Option C: Hybrid Approach
- Keep current multi-engine support
- Add streaming mode for supported engines
- Fallback to process spawning for unsupported engines