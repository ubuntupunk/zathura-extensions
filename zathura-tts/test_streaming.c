/* Test program for streaming TTS engine */

#include "src/tts-streaming-engine.h"
#include <girara/log.h>
#include <stdio.h>
#include <unistd.h>

static void segment_finished_callback(int segment_id, void* user_data) {
    printf("✅ Segment %d finished\n", segment_id);
}

static void state_changed_callback(tts_streaming_state_t old_state, tts_streaming_state_t new_state, void* user_data) {
    const char* state_names[] = {"IDLE", "STARTING", "ACTIVE", "PAUSED", "STOPPING", "ERROR"};
    printf("🔄 State changed: %s → %s\n", 
           state_names[old_state], state_names[new_state]);
}

int main() {
    printf("🧪 Testing Streaming TTS Engine\n");
    printf("================================\n");
    
    /* Initialize logging */
    girara_log_set_level(GIRARA_DEBUG);
    
    /* Create streaming engine */
    printf("1. Creating streaming engine...\n");
    tts_streaming_engine_t* engine = tts_streaming_engine_new(TTS_ENGINE_ESPEAK);
    if (!engine) {
        printf("❌ Failed to create streaming engine\n");
        return 1;
    }
    
    /* Set callbacks */
    tts_streaming_engine_set_segment_finished_callback(engine, segment_finished_callback, NULL);
    tts_streaming_engine_set_state_changed_callback(engine, state_changed_callback, NULL);
    
    /* Start engine */
    printf("2. Starting streaming engine...\n");
    if (!tts_streaming_engine_start(engine)) {
        printf("❌ Failed to start streaming engine\n");
        tts_streaming_engine_free(engine);
        return 1;
    }
    
    /* Queue some test text segments */
    printf("3. Queuing text segments...\n");
    const char* test_texts[] = {
        "This is the first text segment for streaming TTS testing.",
        "Here is the second segment, which should play immediately after the first.",
        "And this is the third segment, demonstrating continuous streaming.",
        "Finally, this last segment shows seamless audio transitions."
    };
    
    for (int i = 0; i < 4; i++) {
        printf("   Queuing segment %d: %.50s...\n", i+1, test_texts[i]);
        if (!tts_streaming_engine_queue_text(engine, test_texts[i], i+1)) {
            printf("❌ Failed to queue segment %d\n", i+1);
        }
        usleep(500000); /* Wait 500ms between segments to simulate real usage */
    }
    
    /* Wait for playback to complete */
    printf("4. Waiting for playback to complete...\n");
    sleep(15); /* Give time for all segments to play */
    
    /* Stop engine */
    printf("5. Stopping streaming engine...\n");
    tts_streaming_engine_stop(engine);
    
    /* Clean up */
    printf("6. Cleaning up...\n");
    tts_streaming_engine_free(engine);
    
    printf("✅ Streaming TTS test completed!\n");
    return 0;
}