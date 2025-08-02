/* TTS UI Controller Implementation
 * Handles keyboard shortcuts and UI integration for TTS functionality
 */

#include "tts-ui-controller.h"
#include "tts-audio-controller.h"
#include "tts-text-extractor.h"
#include <girara/session.h>
#include <girara/statusbar.h>
#include <girara/utils.h>
#include <zathura/zathura.h>
#include <zathura/document.h>
#include <zathura/page.h>
#include <gtk/gtk.h>

/* Global reference to UI controller for shortcut handlers */
static tts_ui_controller_t* g_ui_controller = NULL;

/* Default TTS shortcuts configuration */
static const struct {
    guint modifiers;
    guint key;
    const char* sequence;
    tts_shortcut_action_t action;
    const char* description;
} default_shortcuts[] = {
    { GDK_CONTROL_MASK, GDK_KEY_t, NULL, TTS_SHORTCUT_TOGGLE, "Toggle TTS on/off" },
    { GDK_CONTROL_MASK, GDK_KEY_space, NULL, TTS_SHORTCUT_PAUSE_RESUME, "Pause/resume TTS" },
    { GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_t, NULL, TTS_SHORTCUT_STOP, "Stop TTS" },
    { GDK_CONTROL_MASK, GDK_KEY_Right, NULL, TTS_SHORTCUT_NEXT_SEGMENT, "Next text segment" },
    { GDK_CONTROL_MASK, GDK_KEY_Left, NULL, TTS_SHORTCUT_PREV_SEGMENT, "Previous text segment" },
    { GDK_CONTROL_MASK, GDK_KEY_plus, NULL, TTS_SHORTCUT_SPEED_UP, "Increase TTS speed" },
    { GDK_CONTROL_MASK, GDK_KEY_minus, NULL, TTS_SHORTCUT_SPEED_DOWN, "Decrease TTS speed" },
    { GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_plus, NULL, TTS_SHORTCUT_VOLUME_UP, "Increase TTS volume" },
    { GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_minus, NULL, TTS_SHORTCUT_VOLUME_DOWN, "Decrease TTS volume" },
    { GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_s, NULL, TTS_SHORTCUT_SETTINGS, "TTS settings" }
};

/* UI controller management functions */

tts_ui_controller_t* 
tts_ui_controller_new(zathura_t* zathura, tts_audio_controller_t* audio_controller) 
{
    if (zathura == NULL || audio_controller == NULL) {
        return NULL;
    }
    
    tts_ui_controller_t* controller = g_malloc0(sizeof(tts_ui_controller_t));
    if (controller == NULL) {
        return NULL;
    }
    
    /* Initialize Zathura integration */
    controller->zathura = zathura;
    controller->session = zathura_get_session(zathura);
    
    /* Initialize audio controller reference */
    controller->audio_controller = audio_controller;
    
    /* Initialize shortcut registration state */
    controller->shortcuts_registered = false;
    controller->registered_shortcuts = girara_list_new();
    if (controller->registered_shortcuts != NULL) {
        girara_list_set_free_function(controller->registered_shortcuts, 
                                     (girara_free_function_t)tts_shortcut_info_free);
    }
    
    /* Initialize UI state */
    controller->tts_active = false;
    controller->show_status = true;
    
    /* Initialize status display */
    controller->status_message = NULL;
    controller->status_timeout_id = 0;
    
    /* Set global reference for shortcut handlers */
    g_ui_controller = controller;
    
    return controller;
}

void 
tts_ui_controller_free(tts_ui_controller_t* controller) 
{
    if (controller == NULL) {
        return;
    }
    
    /* Unregister shortcuts */
    tts_ui_controller_unregister_shortcuts(controller);
    
    /* Clear status timeout */
    if (controller->status_timeout_id > 0) {
        g_source_remove(controller->status_timeout_id);
    }
    
    /* Clean up status message */
    g_free(controller->status_message);
    
    /* Clean up shortcuts list */
    if (controller->registered_shortcuts != NULL) {
        girara_list_free(controller->registered_shortcuts);
    }
    
    /* Clear global reference */
    if (g_ui_controller == controller) {
        g_ui_controller = NULL;
    }
    
    g_free(controller);
}

/* Shortcut registration functions */

bool 
tts_ui_controller_register_shortcuts(tts_ui_controller_t* controller) 
{
    if (controller == NULL || controller->session == NULL) {
        return false;
    }
    
    if (controller->shortcuts_registered) {
        return true; /* Already registered */
    }
    
    /* Check for conflicts before registering */
    if (!tts_ui_controller_check_shortcut_conflicts(controller)) {
        tts_ui_controller_show_status(controller, "TTS: Some shortcuts conflict with existing bindings", 3000);
    }
    
    /* Register each shortcut */
    size_t num_shortcuts = sizeof(default_shortcuts) / sizeof(default_shortcuts[0]);
    bool all_registered = true;
    
    for (size_t i = 0; i < num_shortcuts; i++) {
        const auto* shortcut = &default_shortcuts[i];
        
        /* Create shortcut info for tracking */
        tts_shortcut_info_t* info = tts_shortcut_info_new(
            shortcut->modifiers, shortcut->key, shortcut->sequence,
            shortcut->action, shortcut->description
        );
        
        if (info == NULL) {
            continue;
        }
        
        /* Register with Girara based on action type */
        bool registered = false;
        switch (shortcut->action) {
            case TTS_SHORTCUT_TOGGLE:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_toggle, 
                                               0, shortcut->action, NULL);
                break;
            case TTS_SHORTCUT_PAUSE_RESUME:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_pause_resume, 
                                               0, shortcut->action, NULL);
                break;
            case TTS_SHORTCUT_STOP:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_stop, 
                                               0, shortcut->action, NULL);
                break;
            case TTS_SHORTCUT_NEXT_SEGMENT:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_next_segment, 
                                               0, shortcut->action, NULL);
                break;
            case TTS_SHORTCUT_PREV_SEGMENT:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_prev_segment, 
                                               0, shortcut->action, NULL);
                break;
            case TTS_SHORTCUT_SPEED_UP:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_speed_up, 
                                               0, shortcut->action, NULL);
                break;
            case TTS_SHORTCUT_SPEED_DOWN:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_speed_down, 
                                               0, shortcut->action, NULL);
                break;
            case TTS_SHORTCUT_VOLUME_UP:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_volume_up, 
                                               0, shortcut->action, NULL);
                break;
            case TTS_SHORTCUT_VOLUME_DOWN:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_volume_down, 
                                               0, shortcut->action, NULL);
                break;
            case TTS_SHORTCUT_SETTINGS:
                registered = girara_shortcut_add(controller->session, shortcut->modifiers, 
                                               shortcut->key, shortcut->sequence, sc_tts_settings, 
                                               0, shortcut->action, NULL);
                break;
        }
        
        if (registered && controller->registered_shortcuts != NULL) {
            girara_list_append(controller->registered_shortcuts, info);
        } else {
            tts_shortcut_info_free(info);
            all_registered = false;
        }
    }
    
    controller->shortcuts_registered = true;
    
    if (all_registered) {
        tts_ui_controller_show_status(controller, "TTS: Keyboard shortcuts registered", 2000);
    } else {
        tts_ui_controller_show_status(controller, "TTS: Some shortcuts failed to register", 3000);
    }
    
    return all_registered;
}

void 
tts_ui_controller_unregister_shortcuts(tts_ui_controller_t* controller) 
{
    if (controller == NULL || !controller->shortcuts_registered) {
        return;
    }
    
    /* Note: Girara doesn't provide a direct way to unregister shortcuts,
     * so we just clear our tracking list and mark as unregistered */
    if (controller->registered_shortcuts != NULL) {
        girara_list_clear(controller->registered_shortcuts);
    }
    
    controller->shortcuts_registered = false;
}

bool 
tts_ui_controller_check_shortcut_conflicts(tts_ui_controller_t* controller) 
{
    if (controller == NULL) {
        return false;
    }
    
    /* This is a simplified conflict check - in a full implementation,
     * we would check against existing Girara shortcuts */
    
    /* For now, we assume no conflicts and return true */
    /* TODO: Implement proper conflict detection by checking existing shortcuts */
    
    return true;
}

/* Helper functions */

tts_ui_controller_t* 
tts_ui_controller_get_from_session(girara_session_t* session) 
{
    /* Return global reference - in a full implementation, this could be
     * stored in the session's private data */
    return g_ui_controller;
}

tts_shortcut_info_t* 
tts_shortcut_info_new(guint modifiers, guint key, const char* sequence, 
                      tts_shortcut_action_t action, const char* description) 
{
    tts_shortcut_info_t* info = g_malloc0(sizeof(tts_shortcut_info_t));
    if (info == NULL) {
        return NULL;
    }
    
    info->modifiers = modifiers;
    info->key = key;
    info->sequence = sequence ? g_strdup(sequence) : NULL;
    info->action = action;
    info->description = description ? g_strdup(description) : NULL;
    
    return info;
}

void 
tts_shortcut_info_free(tts_shortcut_info_t* info) 
{
    if (info == NULL) {
        return;
    }
    
    g_free(info->sequence);
    g_free(info->description);
    g_free(info);
}

/* Status display functions */

static gboolean 
status_timeout_callback(gpointer user_data) 
{
    tts_ui_controller_t* controller = (tts_ui_controller_t*)user_data;
    if (controller != NULL) {
        tts_ui_controller_clear_status(controller);
    }
    return FALSE; /* Remove timeout */
}

void 
tts_ui_controller_show_status(tts_ui_controller_t* controller, const char* message, int timeout_ms) 
{
    if (controller == NULL || message == NULL || !controller->show_status) {
        return;
    }
    
    /* Clear existing timeout */
    if (controller->status_timeout_id > 0) {
        g_source_remove(controller->status_timeout_id);
        controller->status_timeout_id = 0;
    }
    
    /* Update status message */
    g_free(controller->status_message);
    controller->status_message = g_strdup(message);
    
    /* Display in Zathura's status bar */
    if (controller->session != NULL) {
        girara_statusbar_item_set_text(controller->session, 
                                      girara_statusbar_item_get_default(controller->session), 
                                      message);
    }
    
    /* Set timeout to clear status */
    if (timeout_ms > 0) {
        controller->status_timeout_id = g_timeout_add(timeout_ms, status_timeout_callback, controller);
    }
}

void 
tts_ui_controller_clear_status(tts_ui_controller_t* controller) 
{
    if (controller == NULL) {
        return;
    }
    
    /* Clear timeout */
    if (controller->status_timeout_id > 0) {
        g_source_remove(controller->status_timeout_id);
        controller->status_timeout_id = 0;
    }
    
    /* Clear status message */
    g_free(controller->status_message);
    controller->status_message = NULL;
    
    /* Clear status bar */
    if (controller->session != NULL) {
        girara_statusbar_item_set_text(controller->session, 
                                      girara_statusbar_item_get_default(controller->session), 
                                      "");
    }
}
/* S
hortcut handler functions (following Zathura's pattern) */

bool 
sc_tts_toggle(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    tts_audio_state_t current_state = tts_audio_controller_get_state(controller->audio_controller);
    
    if (current_state == TTS_AUDIO_STATE_STOPPED) {
        /* Start TTS - extract text from current page */
        zathura_document_t* document = zathura_get_document(controller->zathura);
        if (document == NULL) {
            tts_ui_controller_show_status(controller, "TTS: No document loaded", 2000);
            return false;
        }
        
        unsigned int current_page_number = zathura_document_get_current_page_number(document);
        zathura_page_t* page = zathura_document_get_page(document, current_page_number);
        if (page == NULL) {
            tts_ui_controller_show_status(controller, "TTS: Cannot access current page", 2000);
            return false;
        }
        
        /* Extract text segments from current page */
        girara_list_t* segments = tts_extract_page_segments(page, current_page_number);
        if (segments == NULL || girara_list_size(segments) == 0) {
            tts_ui_controller_show_status(controller, "TTS: No readable text found on page", 2000);
            if (segments != NULL) {
                girara_list_free(segments);
            }
            return false;
        }
        
        /* Start TTS session */
        if (tts_audio_controller_start_session(controller->audio_controller, segments)) {
            controller->tts_active = true;
            tts_ui_controller_show_status(controller, "TTS: Started reading", 2000);
            
            /* Start playing current segment */
            tts_audio_controller_play_current_segment(controller->audio_controller);
        } else {
            tts_ui_controller_show_status(controller, "TTS: Failed to start session", 2000);
            girara_list_free(segments);
            return false;
        }
    } else {
        /* Stop TTS */
        tts_audio_controller_stop_session(controller->audio_controller);
        controller->tts_active = false;
        tts_ui_controller_show_status(controller, "TTS: Stopped", 2000);
    }
    
    return true;
}

bool 
sc_tts_pause_resume(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    tts_audio_state_t current_state = tts_audio_controller_get_state(controller->audio_controller);
    
    if (current_state == TTS_AUDIO_STATE_PLAYING) {
        if (tts_audio_controller_pause_session(controller->audio_controller)) {
            tts_ui_controller_show_status(controller, "TTS: Paused", 2000);
        } else {
            tts_ui_controller_show_status(controller, "TTS: Failed to pause", 2000);
            return false;
        }
    } else if (current_state == TTS_AUDIO_STATE_PAUSED) {
        if (tts_audio_controller_resume_session(controller->audio_controller)) {
            tts_ui_controller_show_status(controller, "TTS: Resumed", 2000);
        } else {
            tts_ui_controller_show_status(controller, "TTS: Failed to resume", 2000);
            return false;
        }
    } else {
        tts_ui_controller_show_status(controller, "TTS: Not active", 2000);
        return false;
    }
    
    return true;
}

bool 
sc_tts_stop(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    tts_audio_controller_stop_session(controller->audio_controller);
    controller->tts_active = false;
    tts_ui_controller_show_status(controller, "TTS: Stopped", 2000);
    
    return true;
}

bool 
sc_tts_next_segment(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    if (!controller->tts_active) {
        tts_ui_controller_show_status(controller, "TTS: Not active", 2000);
        return false;
    }
    
    if (tts_audio_controller_navigate_to_segment(controller->audio_controller, 1)) {
        int current_segment = tts_audio_controller_get_current_segment(controller->audio_controller);
        char* status_msg = g_strdup_printf("TTS: Next segment (%d)", current_segment + 1);
        tts_ui_controller_show_status(controller, status_msg, 2000);
        g_free(status_msg);
    } else {
        tts_ui_controller_show_status(controller, "TTS: No next segment", 2000);
        return false;
    }
    
    return true;
}

bool 
sc_tts_prev_segment(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    if (!controller->tts_active) {
        tts_ui_controller_show_status(controller, "TTS: Not active", 2000);
        return false;
    }
    
    if (tts_audio_controller_navigate_to_segment(controller->audio_controller, -1)) {
        int current_segment = tts_audio_controller_get_current_segment(controller->audio_controller);
        char* status_msg = g_strdup_printf("TTS: Previous segment (%d)", current_segment + 1);
        tts_ui_controller_show_status(controller, status_msg, 2000);
        g_free(status_msg);
    } else {
        tts_ui_controller_show_status(controller, "TTS: No previous segment", 2000);
        return false;
    }
    
    return true;
}

bool 
sc_tts_speed_up(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    float current_speed = tts_audio_controller_get_speed(controller->audio_controller);
    float new_speed = current_speed + 0.1f;
    
    if (new_speed > 3.0f) {
        new_speed = 3.0f;
    }
    
    if (tts_audio_controller_set_speed(controller->audio_controller, new_speed)) {
        char* status_msg = g_strdup_printf("TTS: Speed %.1fx", new_speed);
        tts_ui_controller_show_status(controller, status_msg, 2000);
        g_free(status_msg);
    } else {
        tts_ui_controller_show_status(controller, "TTS: Failed to change speed", 2000);
        return false;
    }
    
    return true;
}

bool 
sc_tts_speed_down(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    float current_speed = tts_audio_controller_get_speed(controller->audio_controller);
    float new_speed = current_speed - 0.1f;
    
    if (new_speed < 0.5f) {
        new_speed = 0.5f;
    }
    
    if (tts_audio_controller_set_speed(controller->audio_controller, new_speed)) {
        char* status_msg = g_strdup_printf("TTS: Speed %.1fx", new_speed);
        tts_ui_controller_show_status(controller, status_msg, 2000);
        g_free(status_msg);
    } else {
        tts_ui_controller_show_status(controller, "TTS: Failed to change speed", 2000);
        return false;
    }
    
    return true;
}

bool 
sc_tts_volume_up(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    int current_volume = tts_audio_controller_get_volume(controller->audio_controller);
    int new_volume = current_volume + 10;
    
    if (new_volume > 100) {
        new_volume = 100;
    }
    
    if (tts_audio_controller_set_volume(controller->audio_controller, new_volume)) {
        char* status_msg = g_strdup_printf("TTS: Volume %d%%", new_volume);
        tts_ui_controller_show_status(controller, status_msg, 2000);
        g_free(status_msg);
    } else {
        tts_ui_controller_show_status(controller, "TTS: Failed to change volume", 2000);
        return false;
    }
    
    return true;
}

bool 
sc_tts_volume_down(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    int current_volume = tts_audio_controller_get_volume(controller->audio_controller);
    int new_volume = current_volume - 10;
    
    if (new_volume < 0) {
        new_volume = 0;
    }
    
    if (tts_audio_controller_set_volume(controller->audio_controller, new_volume)) {
        char* status_msg = g_strdup_printf("TTS: Volume %d%%", new_volume);
        tts_ui_controller_show_status(controller, status_msg, 2000);
        g_free(status_msg);
    } else {
        tts_ui_controller_show_status(controller, "TTS: Failed to change volume", 2000);
        return false;
    }
    
    return true;
}

bool 
sc_tts_settings(girara_session_t* session, girara_argument_t* argument, girara_event_t* event, unsigned int t) 
{
    (void)argument;
    (void)event;
    (void)t;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL) {
        return false;
    }
    
    /* For now, just show current settings in status */
    if (controller->audio_controller != NULL) {
        float speed = tts_audio_controller_get_speed(controller->audio_controller);
        int volume = tts_audio_controller_get_volume(controller->audio_controller);
        tts_audio_state_t state = tts_audio_controller_get_state(controller->audio_controller);
        
        const char* state_str = "Stopped";
        switch (state) {
            case TTS_AUDIO_STATE_PLAYING:
                state_str = "Playing";
                break;
            case TTS_AUDIO_STATE_PAUSED:
                state_str = "Paused";
                break;
            case TTS_AUDIO_STATE_ERROR:
                state_str = "Error";
                break;
            default:
                state_str = "Stopped";
                break;
        }
        
        char* status_msg = g_strdup_printf("TTS: %s | Speed: %.1fx | Volume: %d%%", 
                                          state_str, speed, volume);
        tts_ui_controller_show_status(controller, status_msg, 5000);
        g_free(status_msg);
    } else {
        tts_ui_controller_show_status(controller, "TTS: Settings - Audio controller not available", 3000);
    }
    
    return true;
}