# Streaming TTS Engine Test Plan

## Test Environment
- **Zathura**: Running with TTS plugin loaded
- **Test Document**: test.pdf
- **Streaming Engine**: Integrated with audio controller

## Test Sequence

### 1. Baseline Test (Traditional Mode)
**Goal**: Establish baseline performance with current system
**Commands**:
```
:tts-status          # Check current status
Ctrl+T               # Start TTS (traditional mode)
```
**Expected**: Traditional segment-by-segment playback with delays

### 2. Enable Streaming Mode
**Goal**: Switch to streaming engine
**Commands**:
```
:tts-streaming on    # Enable streaming mode
:tts-status          # Verify streaming is enabled
```
**Expected**: "TTS: Streaming mode ENABLED" message

### 3. Test Streaming Performance
**Goal**: Compare streaming vs traditional performance
**Commands**:
```
Ctrl+T               # Start TTS (streaming mode)
```
**Expected**: 
- Continuous audio without gaps between segments
- Single persistent TTS process
- Smoother transitions

### 4. Test Streaming Controls
**Goal**: Verify controls work with streaming
**Commands**:
```
Ctrl+R               # Pause/resume
Ctrl+Shift+T         # Stop
Ctrl+Shift+=         # Speed up
Ctrl+Shift+-         # Speed down
```
**Expected**: Responsive controls without process spawning

### 5. Toggle Between Modes
**Goal**: Test switching between streaming and traditional
**Commands**:
```
:tts-streaming off   # Disable streaming
Ctrl+T               # Test traditional mode
:tts-streaming on    # Re-enable streaming
Ctrl+T               # Test streaming mode
```
**Expected**: Seamless mode switching

## Success Criteria
- ✅ No delays between text segments in streaming mode
- ✅ Single TTS process instead of multiple spawned processes
- ✅ Responsive controls (pause/resume/speed/volume)
- ✅ No threading deadlocks or crashes
- ✅ Smooth mode switching between traditional and streaming

## Performance Comparison
| Metric | Traditional Mode | Streaming Mode |
|--------|------------------|----------------|
| Segment Gaps | ~500ms delays | Continuous |
| Process Count | Multiple spawns | Single persistent |
| Memory Usage | Variable | Stable |
| Responsiveness | Delayed | Immediate |
| Audio Quality | Choppy | Smooth |