# Streaming TTS Architecture Design

## Current Problems
1. **Process spawning per segment** → High overhead, delays between segments
2. **No audio buffering** → Gaps between segments
3. **Complex process management** → Race conditions, cleanup issues

## New Streaming Architecture

### Core Concept: Persistent TTS Service
Instead of spawning processes per segment, maintain a persistent TTS pipeline:

```
Text Segments → Text Queue → TTS Engine → Audio Buffer → Audio Output
     ↓              ↓            ↓           ↓            ↓
  [Segment 1]   [Queue Mgr]  [Persistent]  [Buffer]   [Continuous]
  [Segment 2]      ↓         [Process]       ↓        [Playback]
  [Segment 3]   [Feed Text]     ↓       [Pre-buffer]     ↓
     ...          ↓         [Generate]      ↓         [No Gaps]
                [Async]      [Audio]    [Next Ready]
```

### Implementation Strategy

#### Phase 1: Streaming Text Input
- Create persistent TTS process with stdin pipe
- Feed text segments continuously via pipe
- Eliminate process spawn/kill cycles

#### Phase 2: Audio Buffering  
- Buffer audio output from TTS process
- Start playing current segment while generating next
- Implement seamless transitions

#### Phase 3: Advanced Controls
- Implement proper pause/resume via process signals
- Add speed/volume controls via TTS engine parameters
- Maintain state consistency

## Technical Implementation

### 1. Persistent Process Management
```c
typedef struct {
    GPid process_pid;
    int stdin_fd;      // Pipe to send text
    int stdout_fd;     // Pipe to receive audio
    GIOChannel* text_channel;
    GIOChannel* audio_channel;
    bool is_active;
} tts_streaming_engine_t;
```

### 2. Text Queue Management
```c
typedef struct {
    GQueue* text_queue;
    GMutex queue_mutex;
    GCond queue_cond;
    bool feeding_active;
} tts_text_feeder_t;
```

### 3. Audio Buffer Management
```c
typedef struct {
    GQueue* audio_buffers;
    GMutex buffer_mutex;
    size_t buffer_size;
    bool playback_active;
} tts_audio_buffer_t;
```

## Benefits of New Architecture
1. **No delays between segments** - Continuous audio stream
2. **Better resource usage** - Single persistent process
3. **Smoother controls** - Real-time pause/resume/speed
4. **Simpler management** - No complex process lifecycle
5. **Better error handling** - Single point of failure

## Implementation Plan
1. Create streaming TTS engine interface
2. Implement persistent process management
3. Add text queue and feeding mechanism
4. Implement audio buffering and playback
5. Integrate with existing audio controller
6. Test and optimize performance