/* Speech Dispatcher TTS Engine Implementation
 * System-wide speech synthesis using Speech Dispatcher
 */

#define _DEFAULT_SOURCE
#include "tts-engine-impl.h"

/* Speech Dispatcher engine data structure */
typedef struct {
    GPid current_process;       /**< Current spd-say process PID */
    bool is_speaking;          /**< Whether currently speaking */
    bool is_paused;            /**< Whether currently paused */
    char* current_voice;       /**< Currently selected voice */
    girara_list_t* available_voices; /**< List of available voices */
} speech_dispatcher_engine_data_t;

/* Forward declarations */
static bool speech_dispatcher_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static void speech_dispatcher_engine_cleanup(tts_engine_t* engine);
static bool speech_dispatcher_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error);
static bool speech_dispatcher_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error);
static bool speech_dispatcher_engine_stop(tts_engine_t* engine, zathura_error_t* error);
static bool speech_dispatcher_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static tts_engine_state_t speech_dispatcher_engine_get_state(tts_engine_t* engine);
static girara_list_t* speech_dispatcher_engine_get_voices(tts_engine_t* engine, zathura_error_t* error);

/* Function table */
const tts_engine_functions_t speech_dispatcher_functions = {
    .init = speech_dispatcher_engine_init,
    .cleanup = speech_dispatcher_engine_cleanup,
    .speak = speech_dispatcher_engine_speak,
    .pause = speech_dispatcher_engine_pause,
    .stop = speech_dispatcher_engine_stop,
    .set_config = speech_dispatcher_engine_set_config,
    .get_state = speech_dispatcher_engine_get_state,
    .get_voices = speech_dispatcher_engine_get_voices
};

static bool speech_dispatcher_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    if (engine == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    /* Create Speech Dispatcher engine data */
    speech_dispatcher_engine_data_t* spd_data = g_malloc0(sizeof(speech_dispatcher_engine_data_t));
    if (spd_data == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return false;
    }
    
    spd_data->current_process = 0;
    spd_data->is_speaking = false;
    spd_data->is_paused = false;
    spd_data->current_voice = NULL;
    spd_data->available_voices = NULL;
    
    /* Set default voice if specified in config */
    if (config && config->voice_name) {
        spd_data->current_voice = g_strdup(config->voice_name);
    }
    
    engine->engine_data = spd_data;
    engine->state = TTS_ENGINE_STATE_IDLE;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static void speech_dispatcher_engine_cleanup(tts_engine_t* engine) {
    if (engine == NULL || engine->engine_data == NULL) {
        return;
    }
    
    speech_dispatcher_engine_data_t* spd_data = (speech_dispatcher_engine_data_t*)engine->engine_data;
    
    /* Stop any running process */
    if (spd_data->current_process > 0) {
        kill(spd_data->current_process, SIGTERM);
        waitpid(spd_data->current_process, NULL, 0);
        spd_data->current_process = 0;
    }
    
    /* Free allocated memory */
    g_free(spd_data->current_voice);
    
    if (spd_data->available_voices != NULL) {
        girara_list_free(spd_data->available_voices);
    }
    
    g_free(spd_data);
    engine->engine_data = NULL;
}

static bool speech_dispatcher_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error) {
    if (engine == NULL || text == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    speech_dispatcher_engine_data_t* spd_data = (speech_dispatcher_engine_data_t*)engine->engine_data;
    
    /* Limit text length to prevent extremely long speech */
    char* limited_text = NULL;
    if (strlen(text) > 500) {
        limited_text = g_strndup(text, 500);
        girara_info("🔧 DEBUG: speechd_engine_speak - text truncated from %zu to 500 chars", strlen(text));
        text = limited_text;
    }
    
    /* Stop any current speech */
    if (spd_data->current_process > 0) {
        kill(spd_data->current_process, SIGTERM);
        waitpid(spd_data->current_process, NULL, 0);
        spd_data->current_process = 0;
    }
    
    /* Build spd-say command */
    char* command;
    if (spd_data->current_voice) {
        command = g_strdup_printf("spd-say -v '%s' '%s'", spd_data->current_voice, text);
    } else {
        command = g_strdup_printf("spd-say '%s'", text);
    }
    
    if (command == NULL) {
        if (limited_text) g_free(limited_text);
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return false;
    }
    
    girara_info("🔧 DEBUG: speechd_engine_speak - executing command: %s", command);
    
    /* Execute spd-say command asynchronously */
    GPid child_pid;
    gchar** argv;
    GError* g_error = NULL;
    
    if (!g_shell_parse_argv(command, NULL, &argv, &g_error)) {
        girara_info("🚨 DEBUG: speechd_engine_speak - shell parse failed: %s", 
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
        girara_info("🚨 DEBUG: speechd_engine_speak - spawn failed: %s", 
                    g_error ? g_error->message : "unknown error");
        if (limited_text) g_free(limited_text);
        if (g_error) {
            g_error_free(g_error);
        }
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    girara_info("✅ DEBUG: speechd_engine_speak - spawn successful, PID: %d", child_pid);
    
    spd_data->current_process = child_pid;
    spd_data->is_speaking = true;
    spd_data->is_paused = false;
    engine->state = TTS_ENGINE_STATE_SPEAKING;
    
    /* Clean up limited text if we created it */
    if (limited_text) {
        g_free(limited_text);
    }
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static bool speech_dispatcher_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error) {
    if (engine == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    speech_dispatcher_engine_data_t* spd_data = (speech_dispatcher_engine_data_t*)engine->engine_data;
    
    /* Speech Dispatcher doesn't support pause/resume directly, so we simulate it */
    if (spd_data->current_process > 0) {
        if (pause) {
            /* Send SIGSTOP to pause the process */
            if (kill(spd_data->current_process, SIGSTOP) == 0) {
                spd_data->is_paused = true;
                engine->state = TTS_ENGINE_STATE_PAUSED;
                if (error) *error = ZATHURA_ERROR_OK;
                return true;
            }
        } else {
            /* Send SIGCONT to resume the process */
            if (kill(spd_data->current_process, SIGCONT) == 0) {
                spd_data->is_paused = false;
                engine->state = TTS_ENGINE_STATE_SPEAKING;
                if (error) *error = ZATHURA_ERROR_OK;
                return true;
            }
        }
    }
    
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static bool speech_dispatcher_engine_stop(tts_engine_t* engine, zathura_error_t* error) {
    if (engine == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    speech_dispatcher_engine_data_t* spd_data = (speech_dispatcher_engine_data_t*)engine->engine_data;
    
    /* Stop any running process */
    if (spd_data->current_process > 0) {
        girara_info("🔧 DEBUG: speechd_engine_stop - terminating PID: %d", spd_data->current_process);
        
        /* First try SIGTERM */
        if (kill(spd_data->current_process, SIGTERM) == 0) {
            /* Wait briefly for graceful termination */
            usleep(100000); /* 100ms */
            
            /* Check if process is still running */
            if (kill(spd_data->current_process, 0) == 0) {
                girara_info("🔧 DEBUG: speechd_engine_stop - process still running, using SIGKILL");
                /* Force kill if still running */
                kill(spd_data->current_process, SIGKILL);
            }
            
            waitpid(spd_data->current_process, NULL, 0);
            spd_data->current_process = 0;
            spd_data->is_speaking = false;
            spd_data->is_paused = false;
            engine->state = TTS_ENGINE_STATE_IDLE;
            
            if (error) *error = ZATHURA_ERROR_OK;
            return true;
        }
    }
    
    /* If no process was running, still consider it successful */
    spd_data->is_speaking = false;
    spd_data->is_paused = false;
    engine->state = TTS_ENGINE_STATE_IDLE;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static bool speech_dispatcher_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    if (engine == NULL || config == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    speech_dispatcher_engine_data_t* spd_data = (speech_dispatcher_engine_data_t*)engine->engine_data;
    
    /* Update engine configuration */
    g_free(engine->config.voice_name);
    engine->config = *config;
    engine->config.voice_name = config->voice_name ? g_strdup(config->voice_name) : NULL;
    
    /* Update current voice */
    g_free(spd_data->current_voice);
    spd_data->current_voice = config->voice_name ? g_strdup(config->voice_name) : NULL;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static tts_engine_state_t speech_dispatcher_engine_get_state(tts_engine_t* engine) {
    if (engine == NULL || engine->engine_data == NULL) {
        return TTS_ENGINE_STATE_ERROR;
    }
    
    speech_dispatcher_engine_data_t* spd_data = (speech_dispatcher_engine_data_t*)engine->engine_data;
    
    /* Check if process is still running */
    if (spd_data->current_process > 0) {
        int status;
        pid_t result = waitpid(spd_data->current_process, &status, WNOHANG);
        
        if (result == 0) {
            /* Process is still running */
            return spd_data->is_paused ? TTS_ENGINE_STATE_PAUSED : TTS_ENGINE_STATE_SPEAKING;
        } else {
            /* Process has finished */
            spd_data->current_process = 0;
            spd_data->is_speaking = false;
            spd_data->is_paused = false;
            engine->state = TTS_ENGINE_STATE_IDLE;
            return TTS_ENGINE_STATE_IDLE;
        }
    }
    
    return TTS_ENGINE_STATE_IDLE;
}

static girara_list_t* speech_dispatcher_engine_get_voices(tts_engine_t* engine, zathura_error_t* error) {
    if (engine == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    speech_dispatcher_engine_data_t* spd_data = (speech_dispatcher_engine_data_t*)engine->engine_data;
    
    /* Return cached voices if available */
    if (spd_data->available_voices != NULL) {
        if (error) *error = ZATHURA_ERROR_OK;
        return spd_data->available_voices;
    }
    
    /* Create voices list */
    girara_list_t* voices = girara_list_new();
    if (voices == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    girara_list_set_free_function(voices, (girara_free_function_t)tts_voice_info_free);
    
    /* Add some common Speech Dispatcher voices */
    const char* common_voices[] = {
        "male1", "male2", "male3",
        "female1", "female2", "female3",
        NULL
    };
    
    for (int i = 0; common_voices[i] != NULL; i++) {
        tts_voice_info_t* voice_info = tts_voice_info_new(
            common_voices[i], 
            "en-US", 
            g_str_has_prefix(common_voices[i], "male") ? "male" : "female",
            70  /* Medium quality for system voices */
        );
        
        if (voice_info != NULL) {
            girara_list_append(voices, voice_info);
        }
    }
    
    /* Cache the voices list */
    spd_data->available_voices = voices;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return voices;
}