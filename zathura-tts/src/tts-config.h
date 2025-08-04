#ifndef TTS_CONFIG_H
#define TTS_CONFIG_H

#include <glib.h>
#include <stdbool.h>
#include <girara/session.h>
#include <girara/settings.h>
#include "tts-engine.h"

/* Forward declaration to match plugin.h */
typedef struct tts_config_s tts_config_t;

/* Configuration file paths */
#define TTS_CONFIG_DIR ".config/zathura-tts"
#define TTS_CONFIG_FILE "config"
#define TTS_CONFIG_DEFAULT_PATH "~/.config/zathura-tts/config"

/* Configuration validation limits */
#define TTS_CONFIG_MIN_SPEED 0.5f
#define TTS_CONFIG_MAX_SPEED 3.0f
#define TTS_CONFIG_MIN_VOLUME 0
#define TTS_CONFIG_MAX_VOLUME 100
#define TTS_CONFIG_MIN_PITCH -50
#define TTS_CONFIG_MAX_PITCH 50

/* TTS Configuration structure */
struct tts_config_s {
    /* Engine preferences */
    tts_engine_type_t preferred_engine;
    char* preferred_voice;
    
    /* Audio settings */
    float default_speed;
    int default_volume;
    int default_pitch;
    
    /* Behavior settings */
    bool auto_continue_pages;
    bool highlight_spoken_text;
    bool announce_page_numbers;
    bool announce_headings;
    bool announce_links;
    bool announce_tables;
    
    /* UI settings */
    bool show_status_messages;
    bool show_progress_indicator;
    int status_timeout_ms;
    
    /* Keyboard shortcuts (customizable) */
    char* shortcut_toggle;
    char* shortcut_pause_resume;
    char* shortcut_stop;
    char* shortcut_next_segment;
    char* shortcut_prev_segment;
    char* shortcut_speed_up;
    char* shortcut_speed_down;
    char* shortcut_volume_up;
    char* shortcut_volume_down;
    char* shortcut_settings;
    
    /* Advanced settings */
    bool use_threading;
    int segment_pause_ms;
    bool skip_empty_segments;
    
    /* Configuration metadata */
    char* config_file_path;
    bool is_modified;
    GDateTime* last_modified;
};

/* Configuration management functions */
tts_config_t* tts_config_new(void);
void tts_config_free(tts_config_t* config);
tts_config_t* tts_config_copy(const tts_config_t* config);

/* Configuration file operations */
bool tts_config_load_from_file(tts_config_t* config, const char* file_path);
bool tts_config_save_to_file(const tts_config_t* config, const char* file_path);
bool tts_config_load_default(tts_config_t* config);
bool tts_config_save_default(const tts_config_t* config);

/* Configuration validation */
bool tts_config_validate(const tts_config_t* config, char** error_message);
bool tts_config_validate_speed(float speed);
bool tts_config_validate_volume(int volume);
bool tts_config_validate_pitch(int pitch);
bool tts_config_validate_engine_type(tts_engine_type_t engine_type);

/* Configuration value setters with validation */
bool tts_config_set_preferred_engine(tts_config_t* config, tts_engine_type_t engine);
bool tts_config_set_preferred_voice(tts_config_t* config, const char* voice);
bool tts_config_set_default_speed(tts_config_t* config, float speed);
bool tts_config_set_default_volume(tts_config_t* config, int volume);
bool tts_config_set_default_pitch(tts_config_t* config, int pitch);
bool tts_config_set_auto_continue_pages(tts_config_t* config, bool auto_continue);
bool tts_config_set_highlight_spoken_text(tts_config_t* config, bool highlight);
bool tts_config_set_announce_page_numbers(tts_config_t* config, bool announce);

/* Configuration value getters */
tts_engine_type_t tts_config_get_preferred_engine(const tts_config_t* config);
const char* tts_config_get_preferred_voice(const tts_config_t* config);
float tts_config_get_default_speed(const tts_config_t* config);
int tts_config_get_default_volume(const tts_config_t* config);
int tts_config_get_default_pitch(const tts_config_t* config);
bool tts_config_get_auto_continue_pages(const tts_config_t* config);
bool tts_config_get_highlight_spoken_text(const tts_config_t* config);
bool tts_config_get_announce_page_numbers(const tts_config_t* config);

/* Configuration defaults */
void tts_config_set_defaults(tts_config_t* config);
void tts_config_reset_to_defaults(tts_config_t* config);

/* Configuration file format helpers */
bool tts_config_parse_key_value(const char* line, char** key, char** value);
char* tts_config_format_key_value(const char* key, const char* value);
bool tts_config_create_config_dir(void);
char* tts_config_get_default_path(void);

/* Configuration change tracking */
void tts_config_mark_modified(tts_config_t* config);
bool tts_config_is_modified(const tts_config_t* config);
void tts_config_clear_modified(tts_config_t* config);

/* Zathura configuration integration */
bool tts_config_register_settings(tts_config_t* config, girara_session_t* session);
bool tts_config_load_from_zathura(tts_config_t* config, girara_session_t* session);

#endif /* TTS_CONFIG_H */