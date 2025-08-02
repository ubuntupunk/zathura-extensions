/* TTS Engine Interface Implementation
 * Abstract interface for various TTS engines
 */

#include "tts-engine.h"
#include <string.h>
#include <stdlib.h>
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

/* Forward declarations for engine-specific implementations */
static bool piper_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static void piper_engine_cleanup(tts_engine_t* engine);
static bool piper_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error);
static bool piper_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error);
static bool piper_engine_stop(tts_engine_t* engine, zathura_error_t* error);
static bool piper_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static tts_engine_state_t piper_engine_get_state(tts_engine_t* engine);
static girara_list_t* piper_engine_get_voices(tts_engine_t* engine, zathura_error_t* error);

static bool speech_dispatcher_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static void speech_dispatcher_engine_cleanup(tts_engine_t* engine);
static bool speech_dispatcher_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error);
static bool speech_dispatcher_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error);
static bool speech_dispatcher_engine_stop(tts_engine_t* engine, zathura_error_t* error);
static bool speech_dispatcher_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static tts_engine_state_t speech_dispatcher_engine_get_state(tts_engine_t* engine);
static girara_list_t* speech_dispatcher_engine_get_voices(tts_engine_t* engine, zathura_error_t* error);

static bool espeak_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static void espeak_engine_cleanup(tts_engine_t* engine);
static bool espeak_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error);
static bool espeak_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error);
static bool espeak_engine_stop(tts_engine_t* engine, zathura_error_t* error);
static bool espeak_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
static tts_engine_state_t espeak_engine_get_state(tts_engine_t* engine);
static girara_list_t* espeak_engine_get_voices(tts_engine_t* engine, zathura_error_t* error);

/* Engine function tables */
static const tts_engine_functions_t piper_functions = {
    .init = piper_engine_init,
    .cleanup = piper_engine_cleanup,
    .speak = piper_engine_speak,
    .pause = piper_engine_pause,
    .stop = piper_engine_stop,
    .set_config = piper_engine_set_config,
    .get_state = piper_engine_get_state,
    .get_voices = piper_engine_get_voices
};

static const tts_engine_functions_t speech_dispatcher_functions = {
    .init = speech_dispatcher_engine_init,
    .cleanup = speech_dispatcher_engine_cleanup,
    .speak = speech_dispatcher_engine_speak,
    .pause = speech_dispatcher_engine_pause,
    .stop = speech_dispatcher_engine_stop,
    .set_config = speech_dispatcher_engine_set_config,
    .get_state = speech_dispatcher_engine_get_state,
    .get_voices = speech_dispatcher_engine_get_voices
};

static const tts_engine_functions_t espeak_functions = {
    .init = espeak_engine_init,
    .cleanup = espeak_engine_cleanup,
    .speak = espeak_engine_speak,
    .pause = espeak_engine_pause,
    .stop = espeak_engine_stop,
    .set_config = espeak_engine_set_config,
    .get_state = espeak_engine_get_state,
    .get_voices = espeak_engine_get_voices
};

/* Helper function to check if a command exists */
static bool command_exists(const char* command) {
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
            engine->is_available = command_exists("poetry") || command_exists("piper");
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
    if (engine == NULL || text == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return false;
    }
    
    if (engine->functions.speak == NULL) {
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    return engine->functions.speak(engine, text, error);
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

/* Stub implementations for engine-specific functions */
/* These will be implemented in separate tasks (4.2, 4.3, 4.4) */

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
        command = g_strdup_printf("cd '%s' && echo '%s' | poetry run piper --model '%s' --output-raw | aplay -r 22050 -f S16_LE -t raw -",
                                 project_dir, text, piper_data->model_path);
    } else {
        /* Use default Piper command via Poetry */
        command = g_strdup_printf("cd '%s' && echo '%s' | poetry run piper --output-raw | aplay -r 22050 -f S16_LE -t raw -", 
                                 project_dir, text);
    }
    
    g_free(project_dir);
    
    if (command == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return false;
    }
    
    /* Execute Piper command asynchronously */
    GPid child_pid;
    gchar** argv;
    GError* g_error = NULL;
    
    if (!g_shell_parse_argv(command, NULL, &argv, &g_error)) {
        g_free(command);
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    bool success = g_spawn_async(NULL, argv, NULL, 
                                G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                NULL, NULL, &child_pid, &g_error);
    
    g_strfreev(argv);
    g_free(command);
    
    if (!success) {
        if (g_error) {
            g_error_free(g_error);
        }
        if (error) *error = ZATHURA_ERROR_UNKNOWN;
        return false;
    }
    
    piper_data->current_process = child_pid;
    piper_data->is_speaking = true;
    piper_data->is_paused = false;
    engine->state = TTS_ENGINE_STATE_SPEAKING;
    
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
        if (kill(piper_data->current_process, SIGTERM) == 0) {
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

static bool speech_dispatcher_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    /* TODO: Implement in task 4.3 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static void speech_dispatcher_engine_cleanup(tts_engine_t* engine) {
    /* TODO: Implement in task 4.3 */
}

static bool speech_dispatcher_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error) {
    /* TODO: Implement in task 4.3 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static bool speech_dispatcher_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error) {
    /* TODO: Implement in task 4.3 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static bool speech_dispatcher_engine_stop(tts_engine_t* engine, zathura_error_t* error) {
    /* TODO: Implement in task 4.3 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static bool speech_dispatcher_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    /* TODO: Implement in task 4.3 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static tts_engine_state_t speech_dispatcher_engine_get_state(tts_engine_t* engine) {
    /* TODO: Implement in task 4.3 */
    return TTS_ENGINE_STATE_ERROR;
}

static girara_list_t* speech_dispatcher_engine_get_voices(tts_engine_t* engine, zathura_error_t* error) {
    /* TODO: Implement in task 4.3 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return NULL;
}

static bool espeak_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    /* TODO: Implement in task 4.4 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static void espeak_engine_cleanup(tts_engine_t* engine) {
    /* TODO: Implement in task 4.4 */
}

static bool espeak_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error) {
    /* TODO: Implement in task 4.4 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static bool espeak_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error) {
    /* TODO: Implement in task 4.4 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static bool espeak_engine_stop(tts_engine_t* engine, zathura_error_t* error) {
    /* TODO: Implement in task 4.4 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static bool espeak_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error) {
    /* TODO: Implement in task 4.4 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return false;
}

static tts_engine_state_t espeak_engine_get_state(tts_engine_t* engine) {
    /* TODO: Implement in task 4.4 */
    return TTS_ENGINE_STATE_ERROR;
}

static girara_list_t* espeak_engine_get_voices(tts_engine_t* engine, zathura_error_t* error) {
    /* TODO: Implement in task 4.4 */
    if (error) *error = ZATHURA_ERROR_UNKNOWN;
    return NULL;
}