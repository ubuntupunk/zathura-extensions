#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
    printf("Testing TTS Engine Interface...\n");
    
    /* Test engine type to string conversion */
    printf("Testing engine type to string:\n");
    assert(strcmp(tts_engine_type_to_string(TTS_ENGINE_PIPER), "Piper-TTS") == 0);
    assert(strcmp(tts_engine_type_to_string(TTS_ENGINE_SPEECH_DISPATCHER), "Speech Dispatcher") == 0);
    assert(strcmp(tts_engine_type_to_string(TTS_ENGINE_ESPEAK), "espeak-ng") == 0);
    assert(strcmp(tts_engine_type_to_string(TTS_ENGINE_NONE), "None") == 0);
    printf("✓ Engine type to string works\n");
    
    /* Test voice info creation */
    printf("Testing voice info creation:\n");
    tts_voice_info_t* voice = tts_voice_info_new("test-voice", "en-US", "female", 85);
    assert(voice != NULL);
    assert(strcmp(voice->name, "test-voice") == 0);
    assert(strcmp(voice->language, "en-US") == 0);
    assert(strcmp(voice->gender, "female") == 0);
    assert(voice->quality == 85);
    tts_voice_info_free(voice);
    printf("✓ Voice info creation works\n");
    
    /* Test engine configuration */
    printf("Testing engine configuration:\n");
    tts_engine_config_t* config = tts_engine_config_new();
    assert(config != NULL);
    assert(config->speed == 1.0f);
    assert(config->volume == 80);
    assert(config->pitch == 0);
    assert(config->voice_name == NULL);
    
    tts_engine_config_t* config_copy = tts_engine_config_copy(config);
    assert(config_copy != NULL);
    assert(config_copy->speed == config->speed);
    assert(config_copy->volume == config->volume);
    
    tts_engine_config_free(config);
    tts_engine_config_free(config_copy);
    printf("✓ Engine configuration works\n");
    
    /* Test engine creation */
    printf("Testing engine creation:\n");
    zathura_error_t error;
    
    /* Test invalid engine type */
    tts_engine_t* invalid_engine = tts_engine_new(TTS_ENGINE_NONE, &error);
    assert(invalid_engine == NULL);
    assert(error == ZATHURA_ERROR_INVALID_ARGUMENTS);
    
    /* Test valid engine creation (even if not available) */
    tts_engine_t* engine = tts_engine_new(TTS_ENGINE_PIPER, &error);
    assert(engine != NULL);
    assert(engine->type == TTS_ENGINE_PIPER);
    assert(engine->state == TTS_ENGINE_STATE_IDLE);
    assert(engine->name != NULL);
    assert(strcmp(engine->name, "Piper-TTS") == 0);
    
    /* Test engine state */
    tts_engine_state_t state = tts_engine_get_state(engine);
    assert(state == TTS_ENGINE_STATE_IDLE);
    
    tts_engine_free(engine);
    printf("✓ Engine creation works\n");
    
    /* Test engine detection */
    printf("Testing engine detection:\n");
    girara_list_t* available = tts_engine_detect_available(&error);
    assert(available != NULL);
    printf("✓ Engine detection works (found %zu engines)\n", girara_list_size(available));
    girara_list_free(available);
    
    /* Test preferred engine selection */
    printf("Testing preferred engine selection:\n");
    tts_engine_type_t preferred = tts_engine_get_preferred_type(&error);
    printf("✓ Preferred engine: %s\n", tts_engine_type_to_string(preferred));
    
    printf("\nAll TTS Engine Interface tests passed! ✓\n");
    return 0;
}