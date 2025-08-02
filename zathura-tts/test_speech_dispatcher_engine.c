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
    printf("Testing Speech Dispatcher Engine Implementation...\n");
    
    /* Test Speech Dispatcher engine creation */
    printf("Testing Speech Dispatcher engine creation:\n");
    zathura_error_t error;
    tts_engine_t* engine = tts_engine_new(TTS_ENGINE_SPEECH_DISPATCHER, &error);
    assert(engine != NULL);
    assert(engine->type == TTS_ENGINE_SPEECH_DISPATCHER);
    assert(strcmp(engine->name, "Speech Dispatcher") == 0);
    printf("✓ Speech Dispatcher engine created successfully\n");
    
    /* Test engine initialization */
    printf("Testing Speech Dispatcher engine initialization:\n");
    tts_engine_config_t* config = tts_engine_config_new();
    assert(config != NULL);
    config->voice_name = g_strdup("female1");
    config->speed = 1.2f;
    config->volume = 90;
    config->pitch = 10;
    
    bool init_result = tts_engine_init(engine, config, &error);
    if (engine->is_available) {
        assert(init_result == true);
        printf("✓ Speech Dispatcher engine initialized successfully\n");
    } else {
        printf("⚠ Speech Dispatcher not available on system, skipping initialization test\n");
    }
    
    /* Test engine state */
    printf("Testing Speech Dispatcher engine state:\n");
    tts_engine_state_t state = tts_engine_get_state(engine);
    assert(state == TTS_ENGINE_STATE_IDLE || state == TTS_ENGINE_STATE_ERROR);
    printf("✓ Engine state: %s\n", 
           state == TTS_ENGINE_STATE_IDLE ? "IDLE" : 
           state == TTS_ENGINE_STATE_ERROR ? "ERROR" : "OTHER");
    
    /* Test voice listing */
    printf("Testing Speech Dispatcher voice listing:\n");
    girara_list_t* voices = tts_engine_get_voices(engine, &error);
    if (voices != NULL) {
        size_t voice_count = girara_list_size(voices);
        printf("✓ Found %zu Speech Dispatcher voices\n", voice_count);
        
        /* Print first few voices */
        girara_list_iterator_t* iter = girara_list_iterator(voices);
        int count = 0;
        while (iter != NULL && count < 5) {
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
    printf("Testing Speech Dispatcher configuration update:\n");
    config->speed = 0.8f;
    config->volume = 70;
    config->pitch = -5;
    bool config_result = tts_engine_set_config(engine, config, &error);
    assert(config_result == true);
    assert(engine->config.speed == 0.8f);
    assert(engine->config.volume == 70);
    assert(engine->config.pitch == -5);
    printf("✓ Configuration updated successfully\n");
    
    /* Test speech functionality (if Speech Dispatcher is available) */
    if (engine->is_available) {
        printf("Testing Speech Dispatcher speech functionality:\n");
        
        /* Test speak function */
        bool speak_result = tts_engine_speak(engine, "Hello, this is a test of Speech Dispatcher.", &error);
        if (speak_result) {
            printf("✓ Speech initiated successfully\n");
            
            /* Wait a moment and check state */
            usleep(500000); /* 500ms */
            state = tts_engine_get_state(engine);
            printf("  State after speak: %s\n", 
                   state == TTS_ENGINE_STATE_SPEAKING ? "SPEAKING" :
                   state == TTS_ENGINE_STATE_IDLE ? "IDLE" : "OTHER");
            
            /* Test pause function */
            bool pause_result = tts_engine_pause(engine, true, &error);
            if (pause_result) {
                printf("✓ Speech paused successfully\n");
                usleep(1000000); /* 1 second */
                
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
            printf("⚠ Speech initiation failed (expected if Speech Dispatcher not properly configured)\n");
        }
    } else {
        printf("⚠ Skipping speech tests - Speech Dispatcher not available\n");
    }
    
    /* Test cleanup */
    printf("Testing Speech Dispatcher engine cleanup:\n");
    tts_engine_cleanup(engine);
    tts_engine_free(engine);
    tts_engine_config_free(config);
    printf("✓ Engine cleaned up successfully\n");
    
    printf("\nSpeech Dispatcher Engine tests completed! ✓\n");
    return 0;
}