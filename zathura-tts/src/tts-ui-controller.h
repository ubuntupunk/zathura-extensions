#ifndef TTS_UI_CONTROLLER_H
#define TTS_UI_CONTROLLER_H

#include <glib.h>
#include <stdbool.h>
#include <girara/types.h>
#include <girara/shortcuts.h>
#include <zathura/types.h>

/* Forward declarations */
typedef struct tts_ui_controller_s tts_ui_controller_t;
typedef struct tts_audio_controller_s tts_audio_controller_t;

/* TTS shortcut argument types */
typedef enum {
    TTS_SHORTCUT_TOGGLE,
    TTS_SHORTCUT_PAUSE_RESUME,
    TTS_SHORTCUT_STOP,
    TTS_SHORTCUT_NEXT_SEGMENT,
    TTS_SHORTCUT_PREV_SEGMENT,
    TTS_SHORTCUT_SPEED_UP,
    TTS_SHORTCUT_SPEED_DOWN,
    TTS_SHORTCUT_VOLUME_UP,
    TTS_SHORTCUT_VOLUME_DOWN,
    TTS_SHORTCUT_SETTINGS
} tts_shortcut_action_t;

/* UI controller structure */
struct tts_ui_controller_s {
    /* Zathura integration */
    zathura_t* zathura;
    girara_session_t* session;
    
    /* Audio controller reference */
    tts_audio_controller_t* audio_controller;
    
    /* Shortcut registration state */
    bool shortcuts_registered;
    girara_list_t* registered_shortcuts;
    
    /* UI state */
    bool tts_active;
    bool show_status;
    
    /* Status display */
    char* status_message;
    guint status_timeout_id;
};

/* Shortcut information structure */
typedef struct {
    guint modifiers;
    guint key;
    char* sequence;
    tts_shortcut_action_t action;
    char* description;
} tts_shortcut_info_t;

/* UI controller management functions */
tts_ui_controller_t* tts_ui_controller_new(zathura_t* zathura, tts_audio_controller_t* audio_controller);
void tts_ui_controller_free(tts_ui_controller_t* controller);
bool tts_ui_controller_init_visual_feedback(tts_ui_controller_t* controller);

/* Shortcut registration functions */
bool tts_ui_controller_register_shortcuts(tts_ui_controller_t* controller);
void tts_ui_controller_unregister_shortcuts(tts_ui_controller_t* controller);
bool tts_ui_controller_check_shortcut_conflicts(tts_ui_controller_t* controller);

/* Shortcut handler functions (following Zathura's pattern) */
bool sc_tts_toggle(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);
bool sc_tts_pause_resume(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);
bool sc_tts_stop(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);
bool sc_tts_next_segment(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);
bool sc_tts_prev_segment(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);
bool sc_tts_speed_up(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);
bool sc_tts_speed_down(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);
bool sc_tts_volume_up(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);
bool sc_tts_volume_down(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);
bool sc_tts_settings(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t);

/* Visual feedback functions */
void tts_ui_controller_show_status(tts_ui_controller_t* controller, const char* message, int timeout_ms);
void tts_ui_controller_clear_status(tts_ui_controller_t* controller);
void tts_ui_controller_update_progress(tts_ui_controller_t* controller, int current_segment, int total_segments);
void tts_ui_controller_show_tts_indicator(tts_ui_controller_t* controller, bool active);
bool tts_ui_controller_highlight_current_text(tts_ui_controller_t* controller, const char* text);

/* User notification functions */
bool tts_ui_controller_init_error_handling(tts_ui_controller_t* controller);
bool tts_ui_controller_init_notifications(tts_ui_controller_t* controller);
void tts_ui_controller_notify_engine_switch(tts_ui_controller_t* controller, 
                                           const char* from_engine, 
                                           const char* to_engine,
                                           const char* reason);
void tts_ui_controller_notify_content_unavailable(tts_ui_controller_t* controller, 
                                                 const char* content_type,
                                                 const char* suggestion);
void tts_ui_controller_notify_feature_unavailable(tts_ui_controller_t* controller,
                                                 const char* feature_name,
                                                 const char* reason);
void tts_ui_controller_notify_state_change(tts_ui_controller_t* controller,
                                          tts_audio_state_t old_state,
                                          tts_audio_state_t new_state,
                                          const char* additional_info);
void tts_ui_controller_notify_progress_detailed(tts_ui_controller_t* controller,
                                               int current_page,
                                               int total_pages,
                                               int current_segment,
                                               int total_segments,
                                               const char* current_text_preview);
void tts_ui_controller_notify_config_change(tts_ui_controller_t* controller,
                                           const char* setting_name,
                                           const char* old_value,
                                           const char* new_value);
void tts_ui_controller_notify_system_status(tts_ui_controller_t* controller,
                                           const char* component,
                                           const char* status,
                                           bool is_error);

/* Command interface functions */
bool tts_ui_controller_register_commands(tts_ui_controller_t* controller);
void tts_ui_controller_unregister_commands(tts_ui_controller_t* controller);

/* TTS command functions (following Zathura's pattern) */
bool cmd_tts_toggle(girara_session_t* session, girara_list_t* argument_list);
bool cmd_tts_stop(girara_session_t* session, girara_list_t* argument_list);
bool cmd_tts_speed(girara_session_t* session, girara_list_t* argument_list);
bool cmd_tts_volume(girara_session_t* session, girara_list_t* argument_list);
bool cmd_tts_voice(girara_session_t* session, girara_list_t* argument_list);
bool cmd_tts_engine(girara_session_t* session, girara_list_t* argument_list);
bool cmd_tts_config(girara_session_t* session, girara_list_t* argument_list);
bool cmd_tts_status(girara_session_t* session, girara_list_t* argument_list);

/* Helper functions */
tts_ui_controller_t* tts_ui_controller_get_from_session(girara_session_t* session);
tts_shortcut_info_t* tts_shortcut_info_new(guint modifiers, guint key, const char* sequence, 
                                           tts_shortcut_action_t action, const char* description);
void tts_shortcut_info_free(tts_shortcut_info_t* info);

#endif /* TTS_UI_CONTROLLER_H */