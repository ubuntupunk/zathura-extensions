/* TTS Engine Interface Implementation
 * Abstract interface for various TTS engines
 */

#define _DEFAULT_SOURCE
#include "tts-engine.h"
#include "tts-engine-impl.h"
#include <girara/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

/* Helper function to check if a command exists */
bool command_exists(const char* command) {
    if (command == NULL) {
        return false;
    }
    
    char* test_command = g_strdup_printf("which %s > /dev/null 2>&1", command);
    if (test_command == NULL) {
        return false;
    }
    
    int result = system(test_command);
    g_free(test_command);
    
    return result == 0;
}

tts_engine_t* tts_engine_new(tts_engine_type_t type, zathura_error_t* error) {
    if (type == TTS_ENGINE_NONE) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    tts_engine_t* engine = g_malloc0(sizeof(tts_engine_t));
    if (engine == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    engine->type = type;
    engine->state = TTS_ENGINE_STATE_IDLE;
    engine->is_available = false;
    engine->engine_data = NULL;
    
    /* Set up function pointers based on engine type */
    switch (type) {
        case TTS_ENGINE_PIPER:
            engine->functions = piper_functions;
            engine->name = g_strdup("Piper-TTS");
            /* Check for Poetry-installed Piper first, then system Piper */
            bool piper_command_exists = command_exists("poetry") || command_exists("piper");
            
            /* Also check if any Piper models are available */
            bool piper_models_available = false;
            if (piper_command_exists) {
                /* Check for default model locations */
                char* home_model = g_strdup_printf("%s/.local/share/piper-voices/default.onnx", g_get_home_dir());
                char* project_model = g_strdup_printf("%s/zathura-tts/voices/en_US-lessac-medium.onnx", g_get_current_dir());
                
                piper_models_available = g_file_test(home_model, G_FILE_TEST_EXISTS) || 
                                       g_file_test(project_model, G_FILE_TEST_EXISTS);
                
                g_free(home_model);
                g_free(project_model);
            }
            
            engine->is_available = piper_command_exists && piper_models_available;
            girara_info("🔧 DEBUG: Piper availability - command: %s, models: %s, available: %s",
                       piper_command_exists ? "YES" : "NO",
                       piper_models_available ? "YES" : "NO", 
                       engine->is_available ? "YES" : "NO");
            break;
            
        case TTS_ENGINE_SPEECH_DISPATCHER:
            engine->functions = speech_dispatcher_functions;
            engine->name = g_strdup("Speech Dispatcher");
            engine->is_available = command_exists("spd-say");
            break;
            
        case TTS_ENGINE_ESPEAK:
            engine->functions = espeak_functions;
            engine->name = g_strdup("espeak-ng");
            engine->is_available = command_exists("espeak-ng") || command_exists("espeak");
            break;
            
        case TTS_ENGINE_SYSTEM:
            /* System engine would be implemented based on platform */
            engine->name = g_strdup("System TTS");
            engine->is_available = false; /* Not implemented yet */
            break;
            
        default:
            g_free(engine);
            if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
            return NULL;
    }
    
    /* Initialize default configuration */
    engine->config.speed = 1.0f;
    engine->config.volume = 80;
    engine->config.voice_name = NULL;
    engine->config.pitch = 0;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return engine;
}

void tts_engine_free(tts_engine_t* engine) {
    if (engine == NULL) {
        return;
    }
    
    /* Cleanup engine if it was initialized */
    if (engine->state != TTS_ENGINE_STATE_IDLE) {
        tts_engine_cleanup(engine);
    }
    
    g_free(engine->name);
    g_free(engine->config.voice_name);
    g_free(engine);
}

bool tts_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    if (engine == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    if (!engine->is_available) {
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    /* Apply configuration if provided */
    if (config != NULL) {
        g_free(engine->config.voice_name);
        engine->config = *config;
        engine->config.voice_name = config->voice_name ? g_strdup(config->voice_name) : NULL;
    }
    
    /* Call engine-specific initialization */
    return engine->functions.init(engine, &engine->config, error);
}

void tts_engine_cleanup(tts_engine_t* engine) {
    if (engine == NULL || engine->functions.cleanup == NULL) {
        return;
    }
    
    engine->functions.cleanup(engine);
    engine->state = TTS_ENGINE_STATE_IDLE;
}

bool tts_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error) {
    girara_info("🎤 DEBUG: tts_engine_speak called - engine=%p, text='%.30s%s'", 
                (void*)engine, text ? text : "(null)", text && strlen(text) > 30 ? "..." : "");
    
    if (engine == NULL || text == NULL) {
        girara_info("🚨 DEBUG: tts_engine_speak - invalid arguments: engine=%p, text=%p", 
                    (void*)engine, (void*)text);
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    girara_info("🔧 DEBUG: tts_engine_speak - engine type: %s", 
                engine->name ? engine->name : "unknown");
    
    if (engine->functions.speak == NULL) {
        girara_info("🚨 DEBUG: tts_engine_speak - speak function is NULL for engine %s", 
                    engine->name ? engine->name : "unknown");
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    girara_info("🎤 DEBUG: tts_engine_speak - calling engine-specific speak function...");
    bool result = engine->functions.speak(engine, text, error);
    girara_info("🎤 DEBUG: tts_engine_speak - engine speak result: %s, error: %d", 
                result ? "SUCCESS" : "FAILED", error ? *error : -1);
    
    return result;
}

bool tts_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error) {
    if (engine == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    if (engine->functions.pause == NULL) {
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    return engine->functions.pause(engine, pause, error);
}

bool tts_engine_stop(tts_engine_t* engine, zathura_error_t* error) {
    if (engine == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    if (engine->functions.stop == NULL) {
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    return engine->functions.stop(engine, error);
}

bool tts_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    if (engine == NULL || config == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    if (engine->functions.set_config == NULL) {
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    return engine->functions.set_config(engine, config, error);
}

tts_engine_state_t tts_engine_get_state(tts_engine_t* engine) {
    if (engine == NULL) {
        return TTS_ENGINE_STATE_ERROR;
    }
    
    if (engine->functions.get_state == NULL) {
        return engine->state;
    }
    
    return engine->functions.get_state(engine);
}

girara_list_t* tts_engine_get_voices(tts_engine_t* engine, zathura_error_t* error) {
    if (engine == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    if (engine->functions.get_voices == NULL) {
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return NULL;
    }
    
    return engine->functions.get_voices(engine, error);
}

girara_list_t* tts_engine_detect_available(zathura_error_t* error) {
    girara_list_t* available_engines = girara_list_new();
    if (available_engines == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    /* Check each engine type for availability */
    tts_engine_type_t engine_types[] = {
        TTS_ENGINE_PIPER,
        TTS_ENGINE_SPEECH_DISPATCHER,
        TTS_ENGINE_ESPEAK,
        TTS_ENGINE_SYSTEM
    };
    
    for (size_t i = 0; i < sizeof(engine_types) / sizeof(engine_types[0]); i++) {
        zathura_error_t local_error;
        tts_engine_t* test_engine = tts_engine_new(engine_types[i], &local_error);
        
        if (test_engine != NULL && test_engine->is_available) {
            tts_engine_type_t* type_ptr = g_malloc(sizeof(tts_engine_type_t));
            if (type_ptr != NULL) {
                *type_ptr = engine_types[i];
                girara_list_append(available_engines, type_ptr);
            }
        }
        
        if (test_engine != NULL) {
            tts_engine_free(test_engine);
        }
    }
    
    if (error) *error = ZATHURA_ERROR_OK;
    return available_engines;
}

tts_engine_type_t tts_engine_get_preferred_type(zathura_error_t* error) {
    /* Priority order: Piper > Speech Dispatcher > espeak-ng */
    tts_engine_type_t preferred_order[] = {
        TTS_ENGINE_PIPER,
        TTS_ENGINE_SPEECH_DISPATCHER,
        TTS_ENGINE_ESPEAK,
        TTS_ENGINE_SYSTEM
    };
    
    for (size_t i = 0; i < sizeof(preferred_order) / sizeof(preferred_order[0]); i++) {
        zathura_error_t local_error;
        tts_engine_t* test_engine = tts_engine_new(preferred_order[i], &local_error);
        
        if (test_engine != NULL && test_engine->is_available) {
            tts_engine_type_t result = preferred_order[i];
            tts_engine_free(test_engine);
            if (error) *error = ZATHURA_ERROR_OK;
            return result;
        }
        
        if (test_engine != NULL) {
            tts_engine_free(test_engine);
        }
    }
    
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return TTS_ENGINE_NONE;
}

const char* tts_engine_type_to_string(tts_engine_type_t type) {
    switch (type) {
        case TTS_ENGINE_PIPER:
            return "Piper-TTS";
        case TTS_ENGINE_SPEECH_DISPATCHER:
            return "Speech Dispatcher";
        case TTS_ENGINE_ESPEAK:
            return "espeak-ng";
        case TTS_ENGINE_SYSTEM:
            return "System TTS";
        case TTS_ENGINE_NONE:
            return "None";
        default:
            return "Unknown";
    }
}

tts_voice_info_t* tts_voice_info_new(const char* name, const char* language, 
                                     const char* gender, int quality) {
    if (name == NULL) {
        return NULL;
    }
    
    tts_voice_info_t* voice_info = g_malloc0(sizeof(tts_voice_info_t));
    if (voice_info == NULL) {
        return NULL;
    }
    
    voice_info->name = g_strdup(name);
    voice_info->language = language ? g_strdup(language) : g_strdup("unknown");
    voice_info->gender = gender ? g_strdup(gender) : g_strdup("neutral");
    voice_info->quality = quality;
    
    if (voice_info->name == NULL || voice_info->language == NULL || voice_info->gender == NULL) {
        tts_voice_info_free(voice_info);
        return NULL;
    }
    
    return voice_info;
}

void tts_voice_info_free(tts_voice_info_t* voice_info) {
    if (voice_info == NULL) {
        return;
    }
    
    g_free(voice_info->name);
    g_free(voice_info->language);
    g_free(voice_info->gender);
    g_free(voice_info);
}

tts_engine_config_t* tts_engine_config_new(void) {
    tts_engine_config_t* config = g_malloc0(sizeof(tts_engine_config_t));
    if (config == NULL) {
        return NULL;
    }
    
    /* Set default values */
    config->speed = 1.0f;
    config->volume = 80;
    config->voice_name = NULL;
    config->pitch = 0;
    
    return config;
}

void tts_engine_config_free(tts_engine_config_t* config) {
    if (config == NULL) {
        return;
    }
    
    g_free(config->voice_name);
    g_free(config);
}

tts_engine_config_t* tts_engine_config_copy(const tts_engine_config_t* config) {
    if (config == NULL) {
        return NULL;
    }
    
    tts_engine_config_t* copy = tts_engine_config_new();
    if (copy == NULL) {
        return NULL;
    }
    
    copy->speed = config->speed;
    copy->volume = config->volume;
    copy->pitch = config->pitch;
    copy->voice_name = config->voice_name ? g_strdup(config->voice_name) : NULL;
    
    return copy;
}