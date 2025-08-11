/* espeak-ng TTS Engine Implementation
 * Lightweight text-to-speech using espeak-ng
 */

#define _DEFAULT_SOURCE
#include "tts-engine-impl.h"

/* espeak-ng engine data structure */
typedef struct {
    GPid current_process;       /**< Current espeak-ng process PID */
    bool is_speaking;          /**< Whether currently speaking */
    bool is_paused;            /**< Whether currently paused */
    char* current_voice;       /**< Currently selected voice */
    girara_list_t* available_voices; /**< List of available voices */
} espeak_engine_data_t;

/* Forward declarations */
static bool espeak_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static void espeak_engine_cleanup(tts_engine_t* engine);
static bool espeak_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error);
static bool espeak_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error);
static bool espeak_engine_stop(tts_engine_t* engine, zathura_error_t* error);
static bool espeak_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static tts_engine_state_t espeak_engine_get_state(tts_engine_t* engine);
static girara_list_t* espeak_engine_get_voices(tts_engine_t* engine, zathura_error_t* error);

/* Function table */
const tts_engine_functions_t espeak_functions = {
    .init = espeak_engine_init,
    .cleanup = espeak_engine_cleanup,
    .speak = espeak_engine_speak,
    .pause = espeak_engine_pause,
    .stop = espeak_engine_stop,
    .set_config = espeak_engine_set_config,
    .get_state = espeak_engine_get_state,
    .get_voices = espeak_engine_get_voices
};

static bool espeak_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    if (engine == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    /* Create espeak engine data */
    espeak_engine_data_t* espeak_data = g_malloc0(sizeof(espeak_engine_data_t));
    if (espeak_data == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return false;
    }
    
    espeak_data->current_process = 0;
    espeak_data->is_speaking = false;
    espeak_data->is_paused = false;
    espeak_data->current_voice = NULL;
    espeak_data->available_voices = NULL;
    
    /* Set default voice if specified in config */
    if (config && config->voice_name) {
        espeak_data->current_voice = g_strdup(config->voice_name);
    }
    
    engine->engine_data = espeak_data;
    engine->state = TTS_ENGINE_STATE_IDLE;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static void espeak_engine_cleanup(tts_engine_t* engine) {
    if (engine == NULL || engine->engine_data == NULL) {
        return;
    }
    
    espeak_engine_data_t* espeak_data = (espeak_engine_data_t*)engine->engine_data;
    
    /* Stop any running process */
    if (espeak_data->current_process > 0) {
        kill(espeak_data->current_process, SIGTERM);
        waitpid(espeak_data->current_process, NULL, 0);
        espeak_data->current_process = 0;
    }
    
    /* Free allocated memory */
    g_free(espeak_data->current_voice);
    
    if (espeak_data->available_voices != NULL) {
        girara_list_free(espeak_data->available_voices);
    }
    
    g_free(espeak_data);
    engine->engine_data = NULL;
}

static bool espeak_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error) {
    if (engine == NULL || text == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    espeak_engine_data_t* espeak_data = (espeak_engine_data_t*)engine->engine_data;
    
    /* Limit text length to prevent extremely long speech */
    char* limited_text = NULL;
    if (strlen(text) > 500) {
        limited_text = g_strndup(text, 500);
        girara_info("🔧 DEBUG: espeak_engine_speak - text truncated from %zu to 500 chars", strlen(text));
        text = limited_text;
    }
    
    /* Stop any current speech */
    if (espeak_data->current_process > 0) {
        girara_info("🔧 DEBUG: espeak_engine_speak - stopping previous process PID: %d", espeak_data->current_process);
        
        /* First try SIGTERM */
        if (kill(espeak_data->current_process, SIGTERM) == 0) {
            /* Wait briefly for graceful termination */
            usleep(200000); /* 200ms */
            
            /* Check if process is still running */
            if (kill(espeak_data->current_process, 0) == 0) {
                girara_info("🔧 DEBUG: espeak_engine_speak - process still running, using SIGKILL");
                /* Force kill if still running */
                kill(espeak_data->current_process, SIGKILL);
            }
            
            waitpid(espeak_data->current_process, NULL, 0);
        }
        espeak_data->current_process = 0;
        espeak_data->is_speaking = false;
        girara_info("✅ DEBUG: espeak_engine_speak - previous process stopped");
    }
    
    /* Build espeak-ng command */
    char* command;
    if (espeak_data->current_voice) {
        command = g_strdup_printf("espeak-ng -v '%s' -s %d '%s'", 
                                 espeak_data->current_voice, 
                                 (int)(engine->config.speed * 175), /* espeak speed range */
                                 text);
    } else {
        command = g_strdup_printf("espeak-ng -s %d '%s'", 
                                 (int)(engine->config.speed * 175),
                                 text);
    }
    
    if (command == NULL) {
        if (limited_text) g_free(limited_text);
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return false;
    }
    
    girara_info("🔧 DEBUG: espeak_engine_speak - executing command: %s", command);
    
    /* Execute espeak-ng command asynchronously */
    GPid child_pid;
    gchar** argv;
    GError* g_error = NULL;
    
    if (!g_shell_parse_argv(command, NULL, &argv, &g_error)) {
        girara_info("🚨 DEBUG: espeak_engine_speak - shell parse failed: %s", 
                    g_error ? g_error->message : "unknown error");
        g_free(command);
        if (limited_text) g_free(limited_text);
        if (g_error) g_error_free(g_error);
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    bool success = g_spawn_async(NULL, argv, NULL, 
                                G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                NULL, NULL, &child_pid, &g_error);
    
    g_strfreev(argv);
    g_free(command);
    
    if (!success) {
        girara_info("🚨 DEBUG: espeak_engine_speak - spawn failed: %s", 
                    g_error ? g_error->message : "unknown error");
        if (limited_text) g_free(limited_text);
        if (g_error) {
            g_error_free(g_error);
        }
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    girara_info("✅ DEBUG: espeak_engine_speak - spawn successful, PID: %d", child_pid);
    
    espeak_data->current_process = child_pid;
    espeak_data->is_speaking = true;
    espeak_data->is_paused = false;
    engine->state = TTS_ENGINE_STATE_SPEAKING;
    
    /* Clean up limited text if we created it */
    if (limited_text) {
        g_free(limited_text);
    }
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static bool espeak_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error) {
    if (engine == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    espeak_engine_data_t* espeak_data = (espeak_engine_data_t*)engine->engine_data;
    
    /* espeak-ng doesn't support pause/resume directly, so we simulate it */
    if (espeak_data->current_process > 0) {
        if (pause) {
            /* Send SIGSTOP to pause the process */
            if (kill(espeak_data->current_process, SIGSTOP) == 0) {
                espeak_data->is_paused = true;
                engine->state = TTS_ENGINE_STATE_PAUSED;
                if (error) *error = ZATHURA_ERROR_OK;
                return true;
            }
        } else {
            /* Send SIGCONT to resume the process */
            if (kill(espeak_data->current_process, SIGCONT) == 0) {
                espeak_data->is_paused = false;
                engine->state = TTS_ENGINE_STATE_SPEAKING;
                if (error) *error = ZATHURA_ERROR_OK;
                return true;
            }
        }
    }
    
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static bool espeak_engine_stop(tts_engine_t* engine, zathura_error_t* error) {
    if (engine == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    espeak_engine_data_t* espeak_data = (espeak_engine_data_t*)engine->engine_data;
    
    /* Stop any running process */
    if (espeak_data->current_process > 0) {
        girara_info("🔧 DEBUG: espeak_engine_stop - terminating PID: %d", espeak_data->current_process);
        
        /* First try SIGTERM */
        if (kill(espeak_data->current_process, SIGTERM) == 0) {
            /* Wait briefly for graceful termination */
            usleep(100000); /* 100ms */
            
            /* Check if process is still running */
            if (kill(espeak_data->current_process, 0) == 0) {
                girara_info("🔧 DEBUG: espeak_engine_stop - process still running, using SIGKILL");
                /* Force kill if still running */
                kill(espeak_data->current_process, SIGKILL);
            }
            
            waitpid(espeak_data->current_process, NULL, 0);
            espeak_data->current_process = 0;
            espeak_data->is_speaking = false;
            espeak_data->is_paused = false;
            engine->state = TTS_ENGINE_STATE_IDLE;
            
            if (error) *error = ZATHURA_ERROR_OK;
            return true;
        }
    }
    
    /* If no process was running, still consider it successful */
    espeak_data->is_speaking = false;
    espeak_data->is_paused = false;
    engine->state = TTS_ENGINE_STATE_IDLE;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static bool espeak_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    if (engine == NULL || config == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    espeak_engine_data_t* espeak_data = (espeak_engine_data_t*)engine->engine_data;
    
    /* Update engine configuration */
    g_free(engine->config.voice_name);
    engine->config = *config;
    engine->config.voice_name = config->voice_name ? g_strdup(config->voice_name) : NULL;
    
    /* Update current voice */
    g_free(espeak_data->current_voice);
    espeak_data->current_voice = config->voice_name ? g_strdup(config->voice_name) : NULL;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static tts_engine_state_t espeak_engine_get_state(tts_engine_t* engine) {
    if (engine == NULL || engine->engine_data == NULL) {
        return TTS_ENGINE_STATE_ERROR;
    }
    
    espeak_engine_data_t* espeak_data = (espeak_engine_data_t*)engine->engine_data;
    
    /* Check if process is still running */
    if (espeak_data->current_process > 0) {
        int status;
        pid_t result = waitpid(espeak_data->current_process, &status, WNOHANG);
        
        if (result == 0) {
            /* Process is still running */
            return espeak_data->is_paused ? TTS_ENGINE_STATE_PAUSED : TTS_ENGINE_STATE_SPEAKING;
        } else {
            /* Process has finished */
            espeak_data->current_process = 0;
            espeak_data->is_speaking = false;
            espeak_data->is_paused = false;
            engine->state = TTS_ENGINE_STATE_IDLE;
            return TTS_ENGINE_STATE_IDLE;
        }
    }
    
    return TTS_ENGINE_STATE_IDLE;
}

static girara_list_t* espeak_engine_get_voices(tts_engine_t* engine, zathura_error_t* error) {
    if (engine == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    espeak_engine_data_t* espeak_data = (espeak_engine_data_t*)engine->engine_data;
    
    /* Return cached voices if available */
    if (espeak_data->available_voices != NULL) {
        if (error) *error = ZATHURA_ERROR_OK;
        return espeak_data->available_voices;
    }
    
    /* Create voices list */
    girara_list_t* voices = girara_list_new();
    if (voices == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    girara_list_set_free_function(voices, (girara_free_function_t)tts_voice_info_free);
    
    /* Add some common espeak voices */
    const char* common_voices[] = {
        "en", "en-us", "en-gb", "en-au",
        "es", "fr", "de", "it", "pt",
        NULL
    };
    
    for (int i = 0; common_voices[i] != NULL; i++) {
        tts_voice_info_t* voice_info = tts_voice_info_new(
            common_voices[i], 
            common_voices[i], 
            "neutral",
            60  /* Lower quality for synthetic voices */
        );
        
        if (voice_info != NULL) {
            girara_list_append(voices, voice_info);
        }
    }
    
    /* Cache the voices list */
    espeak_data->available_voices = voices;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return voices;
}