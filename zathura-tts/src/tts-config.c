/* TTS Configuration Implementation
 * Handles TTS settings and preferences with file I/O and validation
 */

#include "tts-config.h"
#include <girara/utils.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <errno.h>

/* Configuration management functions */

tts_config_t* 
tts_config_new(void) 
{
    tts_config_t* config = g_malloc0(sizeof(tts_config_t));
    if (config == NULL) {
        return NULL;
    }
    
    /* Initialize all string pointers to NULL */
    config->preferred_voice = NULL;
    config->shortcut_toggle = NULL;
    config->shortcut_pause_resume = NULL;
    config->shortcut_stop = NULL;
    config->shortcut_next_segment = NULL;
    config->shortcut_prev_segment = NULL;
    config->shortcut_speed_up = NULL;
    config->shortcut_speed_down = NULL;
    config->shortcut_volume_up = NULL;
    config->shortcut_volume_down = NULL;
    config->shortcut_settings = NULL;
    config->config_file_path = NULL;
    config->last_modified = NULL;
    
    /* Set default values */
    tts_config_set_defaults(config);
    
    return config;
}

void 
tts_config_free(tts_config_t* config) 
{
    if (config == NULL) {
        return;
    }
    
    /* Free all allocated strings */
    g_free(config->preferred_voice);
    g_free(config->shortcut_toggle);
    g_free(config->shortcut_pause_resume);
    g_free(config->shortcut_stop);
    g_free(config->shortcut_next_segment);
    g_free(config->shortcut_prev_segment);
    g_free(config->shortcut_speed_up);
    g_free(config->shortcut_speed_down);
    g_free(config->shortcut_volume_up);
    g_free(config->shortcut_volume_down);
    g_free(config->shortcut_settings);
    g_free(config->config_file_path);
    
    if (config->last_modified != NULL) {
        g_date_time_unref(config->last_modified);
    }
    
    g_free(config);
}

tts_config_t* 
tts_config_copy(const tts_config_t* config) 
{
    if (config == NULL) {
        return NULL;
    }
    
    tts_config_t* copy = tts_config_new();
    if (copy == NULL) {
        return NULL;
    }
    
    /* Copy all values */
    copy->preferred_engine = config->preferred_engine;
    copy->preferred_voice = config->preferred_voice ? g_strdup(config->preferred_voice) : NULL;
    
    copy->default_speed = config->default_speed;
    copy->default_volume = config->default_volume;
    copy->default_pitch = config->default_pitch;
    
    copy->auto_continue_pages = config->auto_continue_pages;
    copy->highlight_spoken_text = config->highlight_spoken_text;
    copy->announce_page_numbers = config->announce_page_numbers;
    copy->announce_headings = config->announce_headings;
    copy->announce_links = config->announce_links;
    copy->announce_tables = config->announce_tables;
    
    copy->show_status_messages = config->show_status_messages;
    copy->show_progress_indicator = config->show_progress_indicator;
    copy->status_timeout_ms = config->status_timeout_ms;
    
    copy->shortcut_toggle = config->shortcut_toggle ? g_strdup(config->shortcut_toggle) : NULL;
    copy->shortcut_pause_resume = config->shortcut_pause_resume ? g_strdup(config->shortcut_pause_resume) : NULL;
    copy->shortcut_stop = config->shortcut_stop ? g_strdup(config->shortcut_stop) : NULL;
    copy->shortcut_next_segment = config->shortcut_next_segment ? g_strdup(config->shortcut_next_segment) : NULL;
    copy->shortcut_prev_segment = config->shortcut_prev_segment ? g_strdup(config->shortcut_prev_segment) : NULL;
    copy->shortcut_speed_up = config->shortcut_speed_up ? g_strdup(config->shortcut_speed_up) : NULL;
    copy->shortcut_speed_down = config->shortcut_speed_down ? g_strdup(config->shortcut_speed_down) : NULL;
    copy->shortcut_volume_up = config->shortcut_volume_up ? g_strdup(config->shortcut_volume_up) : NULL;
    copy->shortcut_volume_down = config->shortcut_volume_down ? g_strdup(config->shortcut_volume_down) : NULL;
    copy->shortcut_settings = config->shortcut_settings ? g_strdup(config->shortcut_settings) : NULL;
    
    copy->use_threading = config->use_threading;
    copy->segment_pause_ms = config->segment_pause_ms;
    copy->skip_empty_segments = config->skip_empty_segments;
    
    copy->config_file_path = config->config_file_path ? g_strdup(config->config_file_path) : NULL;
    copy->is_modified = config->is_modified;
    copy->last_modified = config->last_modified ? g_date_time_ref(config->last_modified) : NULL;
    
    return copy;
}

/* Configuration defaults */

void 
tts_config_set_defaults(tts_config_t* config) 
{
    if (config == NULL) {
        return;
    }
    
    /* Engine preferences */
    config->preferred_engine = TTS_ENGINE_PIPER; /* Prefer high-quality neural voices */
    g_free(config->preferred_voice);
    config->preferred_voice = g_strdup("default");
    
    /* Audio settings */
    config->default_speed = 1.0f;
    config->default_volume = 80;
    config->default_pitch = 0;
    
    /* Behavior settings */
    config->auto_continue_pages = true;
    config->highlight_spoken_text = true;
    config->announce_page_numbers = true;
    config->announce_headings = true;
    config->announce_links = true;
    config->announce_tables = true;
    
    /* UI settings */
    config->show_status_messages = true;
    config->show_progress_indicator = true;
    config->status_timeout_ms = 2000;
    
    /* Default keyboard shortcuts */
    g_free(config->shortcut_toggle);
    config->shortcut_toggle = g_strdup("Ctrl+t");
    g_free(config->shortcut_pause_resume);
    config->shortcut_pause_resume = g_strdup("Ctrl+space");
    g_free(config->shortcut_stop);
    config->shortcut_stop = g_strdup("Ctrl+Shift+t");
    g_free(config->shortcut_next_segment);
    config->shortcut_next_segment = g_strdup("Ctrl+Right");
    g_free(config->shortcut_prev_segment);
    config->shortcut_prev_segment = g_strdup("Ctrl+Left");
    g_free(config->shortcut_speed_up);
    config->shortcut_speed_up = g_strdup("Ctrl+plus");
    g_free(config->shortcut_speed_down);
    config->shortcut_speed_down = g_strdup("Ctrl+minus");
    g_free(config->shortcut_volume_up);
    config->shortcut_volume_up = g_strdup("Ctrl+Shift+plus");
    g_free(config->shortcut_volume_down);
    config->shortcut_volume_down = g_strdup("Ctrl+Shift+minus");
    g_free(config->shortcut_settings);
    config->shortcut_settings = g_strdup("Ctrl+Shift+s");
    
    /* Advanced settings */
    config->use_threading = true;
    config->segment_pause_ms = 100;
    config->skip_empty_segments = true;
    
    /* Clear modification flag */
    config->is_modified = false;
}

void 
tts_config_reset_to_defaults(tts_config_t* config) 
{
    if (config == NULL) {
        return;
    }
    
    tts_config_set_defaults(config);
    tts_config_mark_modified(config);
}

/* Configuration validation */

bool 
tts_config_validate_speed(float speed) 
{
    return speed >= TTS_CONFIG_MIN_SPEED && speed <= TTS_CONFIG_MAX_SPEED;
}

bool 
tts_config_validate_volume(int volume) 
{
    return volume >= TTS_CONFIG_MIN_VOLUME && volume <= TTS_CONFIG_MAX_VOLUME;
}

bool 
tts_config_validate_pitch(int pitch) 
{
    return pitch >= TTS_CONFIG_MIN_PITCH && pitch <= TTS_CONFIG_MAX_PITCH;
}

bool 
tts_config_validate_engine_type(tts_engine_type_t engine_type) 
{
    return engine_type >= TTS_ENGINE_PIPER && engine_type < TTS_ENGINE_NONE;
}

bool 
tts_config_validate(const tts_config_t* config, char** error_message) 
{
    if (config == NULL) {
        if (error_message) {
            *error_message = g_strdup("Configuration is NULL");
        }
        return false;
    }
    
    /* Validate speed */
    if (!tts_config_validate_speed(config->default_speed)) {
        if (error_message) {
            *error_message = g_strdup_printf("Invalid speed: %.2f (must be between %.1f and %.1f)", 
                                           config->default_speed, TTS_CONFIG_MIN_SPEED, TTS_CONFIG_MAX_SPEED);
        }
        return false;
    }
    
    /* Validate volume */
    if (!tts_config_validate_volume(config->default_volume)) {
        if (error_message) {
            *error_message = g_strdup_printf("Invalid volume: %d (must be between %d and %d)", 
                                           config->default_volume, TTS_CONFIG_MIN_VOLUME, TTS_CONFIG_MAX_VOLUME);
        }
        return false;
    }
    
    /* Validate pitch */
    if (!tts_config_validate_pitch(config->default_pitch)) {
        if (error_message) {
            *error_message = g_strdup_printf("Invalid pitch: %d (must be between %d and %d)", 
                                           config->default_pitch, TTS_CONFIG_MIN_PITCH, TTS_CONFIG_MAX_PITCH);
        }
        return false;
    }
    
    /* Validate engine type */
    if (!tts_config_validate_engine_type(config->preferred_engine)) {
        if (error_message) {
            *error_message = g_strdup_printf("Invalid engine type: %d", config->preferred_engine);
        }
        return false;
    }
    
    /* Validate timeout */
    if (config->status_timeout_ms < 0) {
        if (error_message) {
            *error_message = g_strdup_printf("Invalid status timeout: %d (must be >= 0)", config->status_timeout_ms);
        }
        return false;
    }
    
    /* Validate segment pause */
    if (config->segment_pause_ms < 0) {
        if (error_message) {
            *error_message = g_strdup_printf("Invalid segment pause: %d (must be >= 0)", config->segment_pause_ms);
        }
        return false;
    }
    
    if (error_message) {
        *error_message = NULL;
    }
    return true;
}

/* Configuration value setters with validation */

bool 
tts_config_set_preferred_engine(tts_config_t* config, tts_engine_type_t engine) 
{
    if (config == NULL || !tts_config_validate_engine_type(engine)) {
        return false;
    }
    
    if (config->preferred_engine != engine) {
        config->preferred_engine = engine;
        tts_config_mark_modified(config);
    }
    
    return true;
}

bool 
tts_config_set_preferred_voice(tts_config_t* config, const char* voice) 
{
    if (config == NULL) {
        return false;
    }
    
    if (g_strcmp0(config->preferred_voice, voice) != 0) {
        g_free(config->preferred_voice);
        config->preferred_voice = voice ? g_strdup(voice) : NULL;
        tts_config_mark_modified(config);
    }
    
    return true;
}

bool 
tts_config_set_default_speed(tts_config_t* config, float speed) 
{
    if (config == NULL || !tts_config_validate_speed(speed)) {
        return false;
    }
    
    if (config->default_speed != speed) {
        config->default_speed = speed;
        tts_config_mark_modified(config);
    }
    
    return true;
}

bool 
tts_config_set_default_volume(tts_config_t* config, int volume) 
{
    if (config == NULL || !tts_config_validate_volume(volume)) {
        return false;
    }
    
    if (config->default_volume != volume) {
        config->default_volume = volume;
        tts_config_mark_modified(config);
    }
    
    return true;
}

bool 
tts_config_set_default_pitch(tts_config_t* config, int pitch) 
{
    if (config == NULL || !tts_config_validate_pitch(pitch)) {
        return false;
    }
    
    if (config->default_pitch != pitch) {
        config->default_pitch = pitch;
        tts_config_mark_modified(config);
    }
    
    return true;
}

bool 
tts_config_set_auto_continue_pages(tts_config_t* config, bool auto_continue) 
{
    if (config == NULL) {
        return false;
    }
    
    if (config->auto_continue_pages != auto_continue) {
        config->auto_continue_pages = auto_continue;
        tts_config_mark_modified(config);
    }
    
    return true;
}

bool 
tts_config_set_highlight_spoken_text(tts_config_t* config, bool highlight) 
{
    if (config == NULL) {
        return false;
    }
    
    if (config->highlight_spoken_text != highlight) {
        config->highlight_spoken_text = highlight;
        tts_config_mark_modified(config);
    }
    
    return true;
}

bool 
tts_config_set_announce_page_numbers(tts_config_t* config, bool announce) 
{
    if (config == NULL) {
        return false;
    }
    
    if (config->announce_page_numbers != announce) {
        config->announce_page_numbers = announce;
        tts_config_mark_modified(config);
    }
    
    return true;
}

/* Configuration value getters */

tts_engine_type_t 
tts_config_get_preferred_engine(const tts_config_t* config) 
{
    return config ? config->preferred_engine : TTS_ENGINE_NONE;
}

const char* 
tts_config_get_preferred_voice(const tts_config_t* config) 
{
    return config ? config->preferred_voice : NULL;
}

float 
tts_config_get_default_speed(const tts_config_t* config) 
{
    return config ? config->default_speed : 1.0f;
}

int 
tts_config_get_default_volume(const tts_config_t* config) 
{
    return config ? config->default_volume : 80;
}

int 
tts_config_get_default_pitch(const tts_config_t* config) 
{
    return config ? config->default_pitch : 0;
}

bool 
tts_config_get_auto_continue_pages(const tts_config_t* config) 
{
    return config ? config->auto_continue_pages : true;
}

bool 
tts_config_get_highlight_spoken_text(const tts_config_t* config) 
{
    return config ? config->highlight_spoken_text : true;
}

bool 
tts_config_get_announce_page_numbers(const tts_config_t* config) 
{
    return config ? config->announce_page_numbers : true;
}/* 
Configuration change tracking */

void 
tts_config_mark_modified(tts_config_t* config) 
{
    if (config == NULL) {
        return;
    }
    
    config->is_modified = true;
    
    if (config->last_modified != NULL) {
        g_date_time_unref(config->last_modified);
    }
    config->last_modified = g_date_time_new_now_local();
}

bool 
tts_config_is_modified(const tts_config_t* config) 
{
    return config ? config->is_modified : false;
}

void 
tts_config_clear_modified(tts_config_t* config) 
{
    if (config == NULL) {
        return;
    }
    
    config->is_modified = false;
}

/* Configuration file format helpers */

bool 
tts_config_parse_key_value(const char* line, char** key, char** value) 
{
    if (line == NULL || key == NULL || value == NULL) {
        return false;
    }
    
    *key = NULL;
    *value = NULL;
    
    /* Skip whitespace */
    while (*line && g_ascii_isspace(*line)) {
        line++;
    }
    
    /* Skip comments and empty lines */
    if (*line == '#' || *line == '\0') {
        return false;
    }
    
    /* Find the equals sign */
    const char* equals = strchr(line, '=');
    if (equals == NULL) {
        return false;
    }
    
    /* Extract key */
    size_t key_len = equals - line;
    while (key_len > 0 && g_ascii_isspace(line[key_len - 1])) {
        key_len--;
    }
    
    if (key_len == 0) {
        return false;
    }
    
    *key = g_strndup(line, key_len);
    
    /* Extract value */
    const char* value_start = equals + 1;
    while (*value_start && g_ascii_isspace(*value_start)) {
        value_start++;
    }
    
    /* Remove trailing whitespace and newline */
    const char* value_end = value_start + strlen(value_start);
    while (value_end > value_start && (g_ascii_isspace(*(value_end - 1)) || *(value_end - 1) == '\n')) {
        value_end--;
    }
    
    *value = g_strndup(value_start, value_end - value_start);
    
    return true;
}

char* 
tts_config_format_key_value(const char* key, const char* value) 
{
    if (key == NULL || value == NULL) {
        return NULL;
    }
    
    return g_strdup_printf("%s = %s\n", key, value);
}

bool 
tts_config_create_config_dir(void) 
{
    char* config_dir = g_strdup_printf("%s/%s", g_get_home_dir(), TTS_CONFIG_DIR);
    if (config_dir == NULL) {
        return false;
    }
    
    /* Create directory if it doesn't exist */
    if (g_mkdir_with_parents(config_dir, 0755) != 0) {
        g_free(config_dir);
        return false;
    }
    
    g_free(config_dir);
    return true;
}

char* 
tts_config_get_default_path(void) 
{
    return g_strdup_printf("%s/%s/%s", g_get_home_dir(), TTS_CONFIG_DIR, TTS_CONFIG_FILE);
}

/* Configuration file operations */

bool 
tts_config_load_from_file(tts_config_t* config, const char* file_path) 
{
    if (config == NULL || file_path == NULL) {
        return false;
    }
    
    FILE* file = fopen(file_path, "r");
    if (file == NULL) {
        return false;
    }
    
    char line[1024];
    bool success = true;
    
    while (fgets(line, sizeof(line), file) != NULL) {
        char* key = NULL;
        char* value = NULL;
        
        if (!tts_config_parse_key_value(line, &key, &value)) {
            continue; /* Skip invalid lines */
        }
        
        /* Parse configuration values */
        if (g_strcmp0(key, "preferred_engine") == 0) {
            if (g_strcmp0(value, "piper") == 0) {
                config->preferred_engine = TTS_ENGINE_PIPER;
            } else if (g_strcmp0(value, "speech_dispatcher") == 0) {
                config->preferred_engine = TTS_ENGINE_SPEECH_DISPATCHER;
            } else if (g_strcmp0(value, "espeak") == 0) {
                config->preferred_engine = TTS_ENGINE_ESPEAK;
            } else if (g_strcmp0(value, "system") == 0) {
                config->preferred_engine = TTS_ENGINE_SYSTEM;
            }
        } else if (g_strcmp0(key, "preferred_voice") == 0) {
            tts_config_set_preferred_voice(config, value);
        } else if (g_strcmp0(key, "default_speed") == 0) {
            float speed = g_ascii_strtod(value, NULL);
            tts_config_set_default_speed(config, speed);
        } else if (g_strcmp0(key, "default_volume") == 0) {
            int volume = atoi(value);
            tts_config_set_default_volume(config, volume);
        } else if (g_strcmp0(key, "default_pitch") == 0) {
            int pitch = atoi(value);
            tts_config_set_default_pitch(config, pitch);
        } else if (g_strcmp0(key, "auto_continue_pages") == 0) {
            bool auto_continue = g_strcmp0(value, "true") == 0;
            tts_config_set_auto_continue_pages(config, auto_continue);
        } else if (g_strcmp0(key, "highlight_spoken_text") == 0) {
            bool highlight = g_strcmp0(value, "true") == 0;
            tts_config_set_highlight_spoken_text(config, highlight);
        } else if (g_strcmp0(key, "announce_page_numbers") == 0) {
            bool announce = g_strcmp0(value, "true") == 0;
            tts_config_set_announce_page_numbers(config, announce);
        } else if (g_strcmp0(key, "announce_headings") == 0) {
            config->announce_headings = g_strcmp0(value, "true") == 0;
        } else if (g_strcmp0(key, "announce_links") == 0) {
            config->announce_links = g_strcmp0(value, "true") == 0;
        } else if (g_strcmp0(key, "announce_tables") == 0) {
            config->announce_tables = g_strcmp0(value, "true") == 0;
        } else if (g_strcmp0(key, "show_status_messages") == 0) {
            config->show_status_messages = g_strcmp0(value, "true") == 0;
        } else if (g_strcmp0(key, "show_progress_indicator") == 0) {
            config->show_progress_indicator = g_strcmp0(value, "true") == 0;
        } else if (g_strcmp0(key, "status_timeout_ms") == 0) {
            config->status_timeout_ms = atoi(value);
        } else if (g_strcmp0(key, "use_threading") == 0) {
            config->use_threading = g_strcmp0(value, "true") == 0;
        } else if (g_strcmp0(key, "segment_pause_ms") == 0) {
            config->segment_pause_ms = atoi(value);
        } else if (g_strcmp0(key, "skip_empty_segments") == 0) {
            config->skip_empty_segments = g_strcmp0(value, "true") == 0;
        }
        /* Add shortcut parsing if needed */
        
        g_free(key);
        g_free(value);
    }
    
    fclose(file);
    
    /* Validate loaded configuration */
    char* error_message = NULL;
    if (!tts_config_validate(config, &error_message)) {
        g_free(error_message);
        success = false;
    }
    
    /* Update file path and clear modified flag */
    g_free(config->config_file_path);
    config->config_file_path = g_strdup(file_path);
    tts_config_clear_modified(config);
    
    return success;
}

bool 
tts_config_save_to_file(const tts_config_t* config, const char* file_path) 
{
    if (config == NULL || file_path == NULL) {
        return false;
    }
    
    /* Validate configuration before saving */
    char* error_message = NULL;
    if (!tts_config_validate(config, &error_message)) {
        g_free(error_message);
        return false;
    }
    
    /* Create config directory if needed */
    char* dir_path = g_path_get_dirname(file_path);
    if (g_mkdir_with_parents(dir_path, 0755) != 0) {
        g_free(dir_path);
        return false;
    }
    g_free(dir_path);
    
    FILE* file = fopen(file_path, "w");
    if (file == NULL) {
        return false;
    }
    
    /* Write configuration header */
    fprintf(file, "# Zathura TTS Configuration\n");
    fprintf(file, "# Generated automatically - edit with care\n\n");
    
    /* Write engine preferences */
    fprintf(file, "# Engine preferences\n");
    const char* engine_name = "piper";
    switch (config->preferred_engine) {
        case TTS_ENGINE_PIPER:
            engine_name = "piper";
            break;
        case TTS_ENGINE_SPEECH_DISPATCHER:
            engine_name = "speech_dispatcher";
            break;
        case TTS_ENGINE_ESPEAK:
            engine_name = "espeak";
            break;
        case TTS_ENGINE_SYSTEM:
            engine_name = "system";
            break;
        default:
            engine_name = "piper";
            break;
    }
    fprintf(file, "preferred_engine = %s\n", engine_name);
    
    if (config->preferred_voice) {
        fprintf(file, "preferred_voice = %s\n", config->preferred_voice);
    }
    fprintf(file, "\n");
    
    /* Write audio settings */
    fprintf(file, "# Audio settings\n");
    fprintf(file, "default_speed = %.2f\n", config->default_speed);
    fprintf(file, "default_volume = %d\n", config->default_volume);
    fprintf(file, "default_pitch = %d\n", config->default_pitch);
    fprintf(file, "\n");
    
    /* Write behavior settings */
    fprintf(file, "# Behavior settings\n");
    fprintf(file, "auto_continue_pages = %s\n", config->auto_continue_pages ? "true" : "false");
    fprintf(file, "highlight_spoken_text = %s\n", config->highlight_spoken_text ? "true" : "false");
    fprintf(file, "announce_page_numbers = %s\n", config->announce_page_numbers ? "true" : "false");
    fprintf(file, "announce_headings = %s\n", config->announce_headings ? "true" : "false");
    fprintf(file, "announce_links = %s\n", config->announce_links ? "true" : "false");
    fprintf(file, "announce_tables = %s\n", config->announce_tables ? "true" : "false");
    fprintf(file, "\n");
    
    /* Write UI settings */
    fprintf(file, "# UI settings\n");
    fprintf(file, "show_status_messages = %s\n", config->show_status_messages ? "true" : "false");
    fprintf(file, "show_progress_indicator = %s\n", config->show_progress_indicator ? "true" : "false");
    fprintf(file, "status_timeout_ms = %d\n", config->status_timeout_ms);
    fprintf(file, "\n");
    
    /* Write advanced settings */
    fprintf(file, "# Advanced settings\n");
    fprintf(file, "use_threading = %s\n", config->use_threading ? "true" : "false");
    fprintf(file, "segment_pause_ms = %d\n", config->segment_pause_ms);
    fprintf(file, "skip_empty_segments = %s\n", config->skip_empty_segments ? "true" : "false");
    
    fclose(file);
    return true;
}

bool 
tts_config_load_default(tts_config_t* config) 
{
    if (config == NULL) {
        return false;
    }
    
    char* default_path = tts_config_get_default_path();
    if (default_path == NULL) {
        return false;
    }
    
    bool success = tts_config_load_from_file(config, default_path);
    g_free(default_path);
    
    return success;
}

bool 
tts_config_save_default(const tts_config_t* config) 
{
    if (config == NULL) {
        return false;
    }
    
    char* default_path = tts_config_get_default_path();
    if (default_path == NULL) {
        return false;
    }
    
    bool success = tts_config_save_to_file(config, default_path);
    g_free(default_path);
    
    return success;
}

/* Zathura configuration integration */

bool 
tts_config_register_settings(tts_config_t* config, girara_session_t* session) 
{
    if (config == NULL || session == NULL) {
        return false;
    }
    
    bool all_registered = true;
    
    /* Register engine preferences */
    char* engine_str = g_strdup("piper");
    all_registered &= girara_setting_add(session, "tts_engine", engine_str, STRING, false,
                                        "TTS engine to use (piper, speech_dispatcher, espeak, system)", NULL, NULL);
    
    char* voice_str = g_strdup("default");
    all_registered &= girara_setting_add(session, "tts_piper_voice", voice_str, STRING, false,
                                        "Piper TTS voice name", NULL, NULL);
    
    char* speechd_voice_str = g_strdup("default");
    all_registered &= girara_setting_add(session, "tts_speechd_voice", speechd_voice_str, STRING, false,
                                        "Speech Dispatcher voice name", NULL, NULL);
    
    char* espeak_voice_str = g_strdup("default");
    all_registered &= girara_setting_add(session, "tts_espeak_voice", espeak_voice_str, STRING, false,
                                        "eSpeak voice name", NULL, NULL);
    
    bool auto_fallback = true;
    all_registered &= girara_setting_add(session, "tts_auto_fallback", &auto_fallback, BOOLEAN, false,
                                        "Automatically fallback to other TTS engines if preferred fails", NULL, NULL);
    
    char* piper_voice_path = g_strdup("");
    all_registered &= girara_setting_add(session, "tts_piper_voice_path", piper_voice_path, STRING, false,
                                        "Path to Piper voice model file", NULL, NULL);
    
    /* Register audio settings */
    float speed = 1.0f;
    all_registered &= girara_setting_add(session, "tts_speed", &speed, FLOAT, false,
                                        "TTS reading speed (0.5-3.0)", NULL, NULL);
    
    int volume = 80;
    all_registered &= girara_setting_add(session, "tts_volume", &volume, INT, false,
                                        "TTS volume level (0-100)", NULL, NULL);
    
    int pitch = 0;
    all_registered &= girara_setting_add(session, "tts_pitch", &pitch, INT, false,
                                        "TTS pitch adjustment (-50 to 50)", NULL, NULL);
    
    /* Register behavior settings */
    bool auto_continue = true;
    all_registered &= girara_setting_add(session, "tts_auto_continue_pages", &auto_continue, BOOLEAN, false,
                                        "Automatically continue reading to next page", NULL, NULL);
    
    bool highlight_text = true;
    all_registered &= girara_setting_add(session, "tts_highlight_text", &highlight_text, BOOLEAN, false,
                                        "Highlight currently spoken text", NULL, NULL);
    
    char* highlight_color = g_strdup("#FFFF00");
    all_registered &= girara_setting_add(session, "tts_highlight_color", highlight_color, STRING, false,
                                        "Color for highlighting spoken text", NULL, NULL);
    
    bool announce_pages = true;
    all_registered &= girara_setting_add(session, "tts_announce_page_numbers", &announce_pages, BOOLEAN, false,
                                        "Announce page numbers when changing pages", NULL, NULL);
    
    bool announce_math = true;
    all_registered &= girara_setting_add(session, "tts_announce_math", &announce_math, BOOLEAN, false,
                                        "Announce mathematical formulas", NULL, NULL);
    
    bool announce_tables = true;
    all_registered &= girara_setting_add(session, "tts_announce_tables", &announce_tables, BOOLEAN, false,
                                        "Announce table structure", NULL, NULL);
    
    bool announce_links = true;
    all_registered &= girara_setting_add(session, "tts_announce_links", &announce_links, BOOLEAN, false,
                                        "Announce hyperlinks", NULL, NULL);
    
    /* Register UI settings */
    bool show_status = true;
    all_registered &= girara_setting_add(session, "tts_show_status", &show_status, BOOLEAN, false,
                                        "Show TTS status messages", NULL, NULL);
    
    bool show_progress = true;
    all_registered &= girara_setting_add(session, "tts_show_progress", &show_progress, BOOLEAN, false,
                                        "Show TTS progress indicator", NULL, NULL);
    
    /* Register advanced settings */
    char* extraction_method = g_strdup("auto");
    all_registered &= girara_setting_add(session, "tts_extraction_method", extraction_method, STRING, false,
                                        "Text extraction method (auto, simple, advanced)", NULL, NULL);
    
    bool optimize_reading = true;
    all_registered &= girara_setting_add(session, "tts_optimize_reading_order", &optimize_reading, BOOLEAN, false,
                                        "Optimize text reading order", NULL, NULL);
    
    int sentence_pause = 100;
    all_registered &= girara_setting_add(session, "tts_sentence_pause", &sentence_pause, INT, false,
                                        "Pause between sentences in milliseconds", NULL, NULL);
    
    int paragraph_pause = 300;
    all_registered &= girara_setting_add(session, "tts_paragraph_pause", &paragraph_pause, INT, false,
                                        "Pause between paragraphs in milliseconds", NULL, NULL);
    
    if (!all_registered) {
        girara_error("Failed to register some TTS configuration options");
        return false;
    }
    
    girara_info("Successfully registered all TTS configuration options with Zathura");
    return true;
}

bool 
tts_config_load_from_zathura(tts_config_t* config, girara_session_t* session) 
{
    if (config == NULL || session == NULL) {
        return false;
    }
    
    /* Load engine preferences */
    char* engine_str = NULL;
    if (girara_setting_get(session, "tts_engine", &engine_str) && engine_str != NULL) {
        if (g_strcmp0(engine_str, "piper") == 0) {
            config->preferred_engine = TTS_ENGINE_PIPER;
        } else if (g_strcmp0(engine_str, "speech_dispatcher") == 0) {
            config->preferred_engine = TTS_ENGINE_SPEECH_DISPATCHER;
        } else if (g_strcmp0(engine_str, "espeak") == 0) {
            config->preferred_engine = TTS_ENGINE_ESPEAK;
        } else if (g_strcmp0(engine_str, "system") == 0) {
            config->preferred_engine = TTS_ENGINE_SYSTEM;
        }
    }
    
    char* voice_str = NULL;
    if (girara_setting_get(session, "tts_piper_voice", &voice_str) && voice_str != NULL) {
        g_free(config->preferred_voice);
        config->preferred_voice = g_strdup(voice_str);
    }
    
    /* Load audio settings */
    float speed;
    if (girara_setting_get(session, "tts_speed", &speed)) {
        if (tts_config_validate_speed(speed)) {
            config->default_speed = speed;
        }
    }
    
    int volume;
    if (girara_setting_get(session, "tts_volume", &volume)) {
        if (tts_config_validate_volume(volume)) {
            config->default_volume = volume;
        }
    }
    
    int pitch;
    if (girara_setting_get(session, "tts_pitch", &pitch)) {
        if (tts_config_validate_pitch(pitch)) {
            config->default_pitch = pitch;
        }
    }
    
    /* Load behavior settings */
    bool auto_continue;
    if (girara_setting_get(session, "tts_auto_continue_pages", &auto_continue)) {
        config->auto_continue_pages = auto_continue;
    }
    
    bool highlight_text;
    if (girara_setting_get(session, "tts_highlight_text", &highlight_text)) {
        config->highlight_spoken_text = highlight_text;
    }
    
    bool announce_pages;
    if (girara_setting_get(session, "tts_announce_page_numbers", &announce_pages)) {
        config->announce_page_numbers = announce_pages;
    }
    
    bool announce_math;
    if (girara_setting_get(session, "tts_announce_math", &announce_math)) {
        config->announce_headings = announce_math; /* Map to headings for now */
    }
    
    bool announce_tables;
    if (girara_setting_get(session, "tts_announce_tables", &announce_tables)) {
        config->announce_tables = announce_tables;
    }
    
    bool announce_links;
    if (girara_setting_get(session, "tts_announce_links", &announce_links)) {
        config->announce_links = announce_links;
    }
    
    /* Load UI settings */
    bool show_status;
    if (girara_setting_get(session, "tts_show_status", &show_status)) {
        config->show_status_messages = show_status;
    }
    
    bool show_progress;
    if (girara_setting_get(session, "tts_show_progress", &show_progress)) {
        config->show_progress_indicator = show_progress;
    }
    
    /* Load advanced settings */
    int sentence_pause;
    if (girara_setting_get(session, "tts_sentence_pause", &sentence_pause)) {
        if (sentence_pause >= 0) {
            config->segment_pause_ms = sentence_pause;
        }
    }
    
    girara_info("Successfully loaded TTS configuration from Zathura settings");
    return true;
}