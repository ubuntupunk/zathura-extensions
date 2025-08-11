# Streaming Mode Default + Piper Integration Test

## Changes Made
1. **Streaming Mode Default**: `use_streaming = true` by default
2. **Piper Streaming Support**: Fixed Poetry-managed Piper command for streaming
3. **Engine Selection**: Streaming uses same engine as regular TTS (Piper preferred)

## Test Plan

### Test 1: Verify Streaming is Default
**Command**: `:tts-status`
**Expected**: Should show streaming mode is enabled by default

### Test 2: Test Piper Streaming
**Command**: `Ctrl+T` (start TTS)
**Expected**: 
- Should use Piper-TTS for streaming
- Poetry-managed Piper command should work
- Continuous audio without gaps

### Test 3: Verify Engine Consistency
**Expected**: Streaming engine should use same type as regular engine

### Test 4: Fallback Testing
**Command**: `:tts-streaming off` then `Ctrl+T`
**Expected**: Should fall back to traditional mode if needed

## Success Criteria
- ✅ Streaming mode enabled by default
- ✅ Piper-TTS working in streaming mode
- ✅ High-quality neural voice audio
- ✅ Continuous playback without delays
- ✅ Single persistent Piper process
- ✅ Backward compatibility maintained

## Expected Log Output
```
🔧 DEBUG: Creating streaming engine with default Piper-TTS
🚀 DEBUG: Starting streaming TTS session with X segments
🔧 DEBUG: Spawning streaming TTS process: sh -c "cd '...' && poetry run piper --model '...' --output-raw"
✅ DEBUG: Streaming TTS engine started successfully
```