#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>

/* Mock Zathura types for testing */
typedef enum {
    ZATHURA_ERROR_OK,
    ZATHURA_ERROR_INVALID_ARGUMENTS,
    ZATHURA_ERROR_OUT_OF_MEMORY,
    ZATHURA_ERROR_UNKNOWN
} zathura_error_t;

/* Include our header */
#include "src/tts-engine.h"

int main() {
    printf("Testing espeak-ng Engine Implementation...\n");
    
    /* Test espeak-ng engine creation */
    printf("Testing espeak-ng engine creation:\n");
    zathura_error_t error;
    tts_engine_t* engine = tts_engine_new(TTS_ENGINE_ESPEAK, &error);
    assert(engine != NULL);
    assert(engine->type == TTS_ENGINE_ESPEAK);
    assert(strcmp(engine->name, "espeak-ng") == 0);
    printf("✓ espeak-ng engine created successfully\n");
    
    /* Test engine initialization */
    printf("Testing espeak-ng engine initialization:\n");
    tts_engine_config_t* config = tts_engine_config_new();
    assert(config != NULL);
    config->voice_name = g_strdup("en-us");
    config->speed = 1.3f;
    config->volume = 85;
    config->pitch = 15;
    
    bool init_result = tts_engine_init(engine, config, &error);
    if (engine->is_available) {
        assert(init_result == true);
        printf("✓ espeak-ng engine initialized successfully\n");
    } else {
        printf("⚠ espeak-ng not available on system, skipping initialization test\n");
    }
    
    /* Test engine state */
    printf("Testing espeak-ng engine state:\n");
    tts_engine_state_t state = tts_engine_get_state(engine);
    assert(state == TTS_ENGINE_STATE_IDLE || state == TTS_ENGINE_STATE_ERROR);
    printf("✓ Engine state: %s\n", 
           state == TTS_ENGINE_STATE_IDLE ? "IDLE" : 
           state == TTS_ENGINE_STATE_ERROR ? "ERROR" : "OTHER");
    
    /* Test voice listing */
    printf("Testing espeak-ng voice listing:\n");
    girara_list_t* voices = tts_engine_get_voices(engine, &error);
    if (voices != NULL) {
        size_t voice_count = girara_list_size(voices);
        printf("✓ Found %zu espeak-ng voices\n", voice_count);
        
        /* Print first few voices */
        girara_list_iterator_t* iter = girara_list_iterator(voices);
        int count = 0;
        while (iter != NULL && count < 8) {
            tts_voice_info_t* voice = (tts_voice_info_t*)girara_list_iterator_data(iter);
            if (voice != NULL) {
                printf("  - %s (%s, %s, quality: %d)\n", 
                       voice->name, voice->language, voice->gender, voice->quality);
            }
            iter = girara_list_iterator_next(iter);
            count++;
        }
    } else {
        printf("⚠ No voices found or error occurred\n");
    }
    
    /* Test configuration update */
    printf("Testing espeak-ng configuration update:\n");
    config->speed = 0.7f;
    config->volume = 95;
    config->pitch = -10;
    bool config_result = tts_engine_set_config(engine, config, &error);
    assert(config_result == true);
    assert(engine->config.speed == 0.7f);
    assert(engine->config.volume == 95);
    assert(engine->config.pitch == -10);
    printf("✓ Configuration updated successfully\n");
    
    /* Test speech functionality (if espeak-ng is available) */
    if (engine->is_available) {
        printf("Testing espeak-ng speech functionality:\n");
        
        /* Test speak function */
        bool speak_result = tts_engine_speak(engine, "Hello, this is a test of espeak-ng.", &error);
        if (speak_result) {
            printf("✓ Speech initiated successfully\n");
            
            /* Wait a moment and check state */
            usleep(300000); /* 300ms */
            state = tts_engine_get_state(engine);
            printf("  State after speak: %s\n", 
                   state == TTS_ENGINE_STATE_SPEAKING ? "SPEAKING" :
                   state == TTS_ENGINE_STATE_IDLE ? "IDLE" : "OTHER");
            
            /* Test pause function */
            bool pause_result = tts_engine_pause(engine, true, &error);
            if (pause_result) {
                printf("✓ Speech paused successfully\n");
                usleep(800000); /* 800ms */
                
                /* Test resume function */
                bool resume_result = tts_engine_pause(engine, false, &error);
                if (resume_result) {
                    printf("✓ Speech resumed successfully\n");
                }
            }
            
            /* Test stop function */
            bool stop_result = tts_engine_stop(engine, &error);
            assert(stop_result == true);
            printf("✓ Speech stopped successfully\n");
        } else {
            printf("⚠ Speech initiation failed (expected if espeak-ng not properly configured)\n");
        }
    } else {
        printf("⚠ Skipping speech tests - espeak-ng not available\n");
    }
    
    /* Test cleanup */
    printf("Testing espeak-ng engine cleanup:\n");
    tts_engine_cleanup(engine);
    tts_engine_free(engine);
    tts_engine_config_free(config);
    printf("✓ Engine cleaned up successfully\n");
    
    printf("\nespeak-ng Engine tests completed! ✓\n");
    return 0;
}