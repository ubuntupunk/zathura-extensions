/* Piper TTS Engine Implementation
 * Neural text-to-speech using Piper
 */

#define _DEFAULT_SOURCE
#include "tts-engine.h"
#include <girara/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

/* Piper-TTS engine data structure */
typedef struct {
    GPid current_process;       /**< Current Piper process PID */
    bool is_speaking;          /**< Whether currently speaking */
    bool is_paused;            /**< Whether currently paused */
    char* model_path;          /**< Path to Piper model */
    char* config_path;         /**< Path to Piper config */
    girara_list_t* available_voices; /**< List of available voices */
} piper_engine_data_t;

/* Forward declarations */
static bool piper_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static void piper_engine_cleanup(tts_engine_t* engine);
static bool piper_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error);
static bool piper_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error);
static bool piper_engine_stop(tts_engine_t* engine, zathura_error_t* error);
static bool piper_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static tts_engine_state_t piper_engine_get_state(tts_engine_t* engine);
static girara_list_t* piper_engine_get_voices(tts_engine_t* engine, zathura_error_t* error);

/* Engine function table */
const tts_engine_functions_t piper_functions = {
    .init = piper_engine_init,
    .cleanup = piper_engine_cleanup,
    .speak = piper_engine_speak,
    .pause = piper_engine_pause,
    .stop = piper_engine_stop,
    .set_config = piper_engine_set_config,
    .get_state = piper_engine_get_state,
    .get_voices = piper_engine_get_voices
};

static bool piper_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    if (engine == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    /* Create Piper engine data */
    piper_engine_data_t* piper_data = g_malloc0(sizeof(piper_engine_data_t));
    if (piper_data == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return false;
    }
    
    piper_data->current_process = 0;
    piper_data->is_speaking = false;
    piper_data->is_paused = false;
    piper_data->model_path = NULL;
    piper_data->config_path = NULL;
    piper_data->available_voices = NULL;
    
    /* Set default model path if voice is specified */
    if (config && config->voice_name) {
        piper_data->model_path = g_strdup_printf("%s/.local/share/piper-voices/%s.onnx", 
                                                g_get_home_dir(), config->voice_name);
        piper_data->config_path = g_strdup_printf("%s/.local/share/piper-voices/%s.onnx.json", 
                                                 g_get_home_dir(), config->voice_name);
    } else {
        /* Use default model from voices directory */
        char* current_dir = g_get_current_dir();
        char* project_dir = g_strdup_printf("%s/zathura-tts", current_dir);
        piper_data->model_path = g_strdup_printf("%s/voices/en_US-lessac-medium.onnx", project_dir);
        piper_data->config_path = g_strdup_printf("%s/voices/en_US-lessac-medium.onnx.json", project_dir);
        g_free(current_dir);
        g_free(project_dir);
    }
    
    engine->engine_data = piper_data;
    engine->state = TTS_ENGINE_STATE_IDLE;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static void piper_engine_cleanup(tts_engine_t* engine) {
    if (engine == NULL || engine->engine_data == NULL) {
        return;
    }
    
    piper_engine_data_t* piper_data = (piper_engine_data_t*)engine->engine_data;
    
    /* Stop any running process */
    if (piper_data->current_process > 0) {
        kill(piper_data->current_process, SIGTERM);
        waitpid(piper_data->current_process, NULL, 0);
        piper_data->current_process = 0;
    }
    
    /* Free allocated memory */
    g_free(piper_data->model_path);
    g_free(piper_data->config_path);
    
    if (piper_data->available_voices != NULL) {
        girara_list_free(piper_data->available_voices);
    }
    
    g_free(piper_data);
    engine->engine_data = NULL;
}

static bool piper_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error) {
    if (engine == NULL || text == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    piper_engine_data_t* piper_data = (piper_engine_data_t*)engine->engine_data;
    
    /* Limit text length to prevent extremely long speech */
    char* limited_text = NULL;
    if (strlen(text) > 500) {
        limited_text = g_strndup(text, 500);
        girara_info("🔧 DEBUG: piper_engine_speak - text truncated from %zu to 500 chars", strlen(text));
        text = limited_text;
    }
    
    girara_info("🔧 DEBUG: piper_engine_speak - model_path: %s", 
                piper_data->model_path ? piper_data->model_path : "(null)");
    
    /* Stop any current speech */
    if (piper_data->current_process > 0) {
        kill(piper_data->current_process, SIGTERM);
        waitpid(piper_data->current_process, NULL, 0);
        piper_data->current_process = 0;
    }
    
    /* Build Piper command - prefer Poetry-installed version */
    char* command;
    char* current_dir = g_get_current_dir();
    char* project_dir = g_strdup_printf("%s/zathura-tts", current_dir);
    g_free(current_dir);
    
    if (piper_data->model_path && g_file_test(piper_data->model_path, G_FILE_TEST_EXISTS)) {
        /* Use specific model if available */
        girara_info("🔧 DEBUG: piper_engine_speak - using model: %s", piper_data->model_path);
        command = g_strdup_printf("sh -c \"cd '%s' && echo '%s' | poetry run piper --model '%s' --output-raw | aplay -r 22050 -f S16_LE -t raw -\"",
                                 project_dir, text, piper_data->model_path);
    } else {
        /* Model not found - this will fail since Piper requires a model */
        girara_info("🚨 DEBUG: piper_engine_speak - no model found at: %s", 
                    piper_data->model_path ? piper_data->model_path : "(null)");
        girara_info("🚨 DEBUG: piper_engine_speak - Piper requires a model file, this will fail");
        if (limited_text) g_free(limited_text);
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        g_free(project_dir);
        return false;
    }
    
    g_free(project_dir);
    
    if (command == NULL) {
        if (limited_text) g_free(limited_text);
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return false;
    }
    
    girara_info("🔧 DEBUG: piper_engine_speak - executing command: %s", command);
    
    /* Execute Piper command asynchronously */
    GPid child_pid;
    gchar** argv;
    GError* g_error = NULL;
    
    if (!g_shell_parse_argv(command, NULL, &argv, &g_error)) {
        girara_info("🚨 DEBUG: piper_engine_speak - shell parse failed: %s", 
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
        girara_info("🚨 DEBUG: piper_engine_speak - spawn failed: %s", 
                    g_error ? g_error->message : "unknown error");
        if (limited_text) g_free(limited_text);
        if (g_error) {
            g_error_free(g_error);
        }
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    girara_info("✅ DEBUG: piper_engine_speak - spawn successful, PID: %d", child_pid);
    
    piper_data->current_process = child_pid;
    piper_data->is_speaking = true;
    piper_data->is_paused = false;
    engine->state = TTS_ENGINE_STATE_SPEAKING;
    
    /* Clean up limited text if we created it */
    if (limited_text) {
        g_free(limited_text);
    }
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static bool piper_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error) {
    if (engine == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    piper_engine_data_t* piper_data = (piper_engine_data_t*)engine->engine_data;
    
    /* Note: Piper doesn't support pause/resume directly, so we simulate it */
    if (piper_data->current_process > 0) {
        if (pause) {
            /* Send SIGSTOP to pause the process */
            if (kill(piper_data->current_process, SIGSTOP) == 0) {
                piper_data->is_paused = true;
                engine->state = TTS_ENGINE_STATE_PAUSED;
                if (error) *error = ZATHURA_ERROR_OK;
                return true;
            }
        } else {
            /* Send SIGCONT to resume the process */
            if (kill(piper_data->current_process, SIGCONT) == 0) {
                piper_data->is_paused = false;
                engine->state = TTS_ENGINE_STATE_SPEAKING;
                if (error) *error = ZATHURA_ERROR_OK;
                return true;
            }
        }
    }
    
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static bool piper_engine_stop(tts_engine_t* engine, zathura_error_t* error) {
    if (engine == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    piper_engine_data_t* piper_data = (piper_engine_data_t*)engine->engine_data;
    
    /* Stop any running process */
    if (piper_data->current_process > 0) {
        girara_info("🔧 DEBUG: piper_engine_stop - terminating PID: %d", piper_data->current_process);
        
        /* First try SIGTERM */
        if (kill(piper_data->current_process, SIGTERM) == 0) {
            /* Wait briefly for graceful termination */
            usleep(100000); /* 100ms */
            
            /* Check if process is still running */
            if (kill(piper_data->current_process, 0) == 0) {
                girara_info("🔧 DEBUG: piper_engine_stop - process still running, using SIGKILL");
                /* Force kill if still running */
                kill(piper_data->current_process, SIGKILL);
            }
            
            waitpid(piper_data->current_process, NULL, 0);
            piper_data->current_process = 0;
            piper_data->is_speaking = false;
            piper_data->is_paused = false;
            engine->state = TTS_ENGINE_STATE_IDLE;
            
            if (error) *error = ZATHURA_ERROR_OK;
            return true;
        }
    }
    
    /* If no process was running, still consider it successful */
    piper_data->is_speaking = false;
    piper_data->is_paused = false;
    engine->state = TTS_ENGINE_STATE_IDLE;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static bool piper_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    if (engine == NULL || config == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    piper_engine_data_t* piper_data = (piper_engine_data_t*)engine->engine_data;
    
    /* Update engine configuration */
    g_free(engine->config.voice_name);
    engine->config = *config;
    engine->config.voice_name = config->voice_name ? g_strdup(config->voice_name) : NULL;
    
    /* Update model paths if voice changed */
    if (config->voice_name) {
        g_free(piper_data->model_path);
        g_free(piper_data->config_path);
        
        piper_data->model_path = g_strdup_printf("%s/.local/share/piper-voices/%s.onnx", 
                                                g_get_home_dir(), config->voice_name);
        piper_data->config_path = g_strdup_printf("%s/.local/share/piper-voices/%s.onnx.json", 
                                                 g_get_home_dir(), config->voice_name);
    }
    
    if (error) *error = ZATHURA_ERROR_OK;
    return true;
}

static tts_engine_state_t piper_engine_get_state(tts_engine_t* engine) {
    if (engine == NULL || engine->engine_data == NULL) {
        return TTS_ENGINE_STATE_ERROR;
    }
    
    piper_engine_data_t* piper_data = (piper_engine_data_t*)engine->engine_data;
    
    /* Check if process is still running */
    if (piper_data->current_process > 0) {
        int status;
        pid_t result = waitpid(piper_data->current_process, &status, WNOHANG);
        
        if (result == 0) {
            /* Process is still running */
            return piper_data->is_paused ? TTS_ENGINE_STATE_PAUSED : TTS_ENGINE_STATE_SPEAKING;
        } else {
            /* Process has finished */
            piper_data->current_process = 0;
            piper_data->is_speaking = false;
            piper_data->is_paused = false;
            engine->state = TTS_ENGINE_STATE_IDLE;
            return TTS_ENGINE_STATE_IDLE;
        }
    }
    
    return TTS_ENGINE_STATE_IDLE;
}

static girara_list_t* piper_engine_get_voices(tts_engine_t* engine, zathura_error_t* error) {
    if (engine == NULL || engine->engine_data == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    piper_engine_data_t* piper_data = (piper_engine_data_t*)engine->engine_data;
    
    /* Return cached voices if available */
    if (piper_data->available_voices != NULL) {
        if (error) *error = ZATHURA_ERROR_OK;
        return piper_data->available_voices;
    }
    
    /* Create voices list */
    girara_list_t* voices = girara_list_new();
    if (voices == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    girara_list_set_free_function(voices, (girara_free_function_t)tts_voice_info_free);
    
    /* Check for available Piper voices in standard locations */
    const char* voice_dirs[] = {
        g_strdup_printf("%s/.local/share/piper-voices", g_get_home_dir()),
        "/usr/share/piper-voices",
        "/usr/local/share/piper-voices",
        NULL
    };
    
    for (int i = 0; voice_dirs[i] != NULL; i++) {
        GDir* dir = g_dir_open(voice_dirs[i], 0, NULL);
        if (dir != NULL) {
            const char* filename;
            while ((filename = g_dir_read_name(dir)) != NULL) {
                /* Look for .onnx model files */
                if (g_str_has_suffix(filename, ".onnx")) {
                    /* Extract voice name (remove .onnx extension) */
                    char* voice_name = g_strndup(filename, strlen(filename) - 5);
                    if (voice_name != NULL) {
                        /* Create voice info with basic metadata */
                        tts_voice_info_t* voice_info = tts_voice_info_new(
                            voice_name, 
                            "en-US",  /* Default language - could be parsed from filename */
                            "neutral", /* Default gender */
                            85        /* High quality for neural voices */
                        );
                        
                        if (voice_info != NULL) {
                            girara_list_append(voices, voice_info);
                        }
                        
                        g_free(voice_name);
                    }
                }
            }
            g_dir_close(dir);
        }
        g_free((char*)voice_dirs[i]);
    }
    
    /* If no voices found, add a default entry */
    if (girara_list_size(voices) == 0) {
        tts_voice_info_t* default_voice = tts_voice_info_new(
            "default", 
            "en-US", 
            "neutral", 
            75
        );
        if (default_voice != NULL) {
            girara_list_append(voices, default_voice);
        }
    }
    
    /* Cache the voices list */
    piper_data->available_voices = voices;
    
    if (error) *error = ZATHURA_ERROR_OK;
    return voices;
}