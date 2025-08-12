/* TTS UI Controller Implementation
 * Handles keyboard shortcuts and UI integration for TTS functionality
 */

#include "tts-ui-controller.h"
#include "tts-audio-controller.h"
#include "tts-text-extractor.h"
#include "tts-error.h"
#include "zathura-plugin.h"
#include <girara/session.h>
#include <girara/statusbar.h>
#include <girara/utils.h>
#include <girara/commands.h>
#include <girara/shortcuts.h>
#include <girara/log.h>
#include <zathura/types.h>
#include <zathura/plugin-api.h>
#include <zathura/document.h>
#include <zathura/page.h>
#include <gtk/gtk.h>

/* Global reference to UI controller for shortcut handlers */
static tts_ui_controller_t* g_ui_controller = NULL;

/* Forward declarations for command functions */

/* Default TTS shortcuts configuration */
static const struct {
    guint modifiers;
    guint key;
    const char* sequence;
    tts_shortcut_action_t action;
    const char* description;
} default_shortcuts[] = {
    { GDK_CONTROL_MASK, GDK_KEY_t, NULL, TTS_SHORTCUT_TOGGLE, "Toggle TTS on/off" },
    { GDK_CONTROL_MASK, GDK_KEY_r, NULL, TTS_SHORTCUT_PAUSE_RESUME, "Pause/resume TTS" },
    { GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_t, NULL, TTS_SHORTCUT_STOP, "Stop TTS" },
    { GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_Right, NULL, TTS_SHORTCUT_NEXT_SEGMENT, "Next text segment" },
    { GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_Left, NULL, TTS_SHORTCUT_PREV_SEGMENT, "Previous text segment" },
    { GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_equal, NULL, TTS_SHORTCUT_SPEED_UP, "Increase TTS speed" },
    { GDK_CONTROL_MASK | GDK_SHIFT_MASK, GDK_KEY_minus, NULL, TTS_SHORTCUT_SPEED_DOWN, "Decrease TTS speed" },
    { GDK_CONTROL_MASK | GDK_MOD1_MASK, GDK_KEY_equal, NULL, TTS_SHORTCUT_VOLUME_UP, "Increase TTS volume" },
    { GDK_CONTROL_MASK | GDK_MOD1_MASK, GDK_KEY_minus, NULL, TTS_SHORTCUT_VOLUME_DOWN, "Decrease TTS volume" },
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
    
    girara_info("🔧 DEBUG: UI controller created - session: %p", (void*)controller->session);
    
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
    girara_info("🔧 DEBUG: tts_ui_controller_register_shortcuts called - controller: %p", (void*)controller);
    if (controller == NULL || controller->session == NULL) {
        girara_info("❌ DEBUG: Registration failed - controller: %p, session: %p", (void*)controller, (void*)(controller ? controller->session : NULL));
        return false;
    }
    girara_info("✅ DEBUG: Controller and session are valid, proceeding with registration...");
    
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
    
    girara_info("🔧 DEBUG: Registering %zu TTS shortcuts...", num_shortcuts);
    
    for (size_t i = 0; i < num_shortcuts; i++) {
        const tts_shortcut_t* shortcut = &default_shortcuts[i];
        
        girara_debug("🔧 DEBUG: Registering shortcut %zu: %s (action %d)", i, shortcut->description, shortcut->action);
        
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
            girara_debug("✅ DEBUG: Successfully registered shortcut: %s", shortcut->description);
            girara_list_append(controller->registered_shortcuts, info);
        } else {
            girara_debug("❌ DEBUG: Failed to register shortcut: %s", shortcut->description);
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
        girara_notify(controller->session, GIRARA_INFO, "TTS: %s", message);
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
        girara_notify(controller->session, GIRARA_INFO, "");
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
    
    girara_info("🎯 DEBUG: sc_tts_toggle called - Ctrl+T pressed!");
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        girara_info("🚨 DEBUG: sc_tts_toggle - controller or audio_controller is NULL");
        return false;
    }
    
    girara_info("✅ DEBUG: sc_tts_toggle - controller found, checking current state...");
    
    tts_audio_state_t current_state = tts_audio_controller_get_state(controller->audio_controller);
    girara_info("🔍 DEBUG: sc_tts_toggle - current audio state: %d", current_state);
    
    if (current_state == TTS_AUDIO_STATE_STOPPED) {
        girara_info("🔍 DEBUG: sc_tts_toggle - starting TTS, getting document...");
        
        /* Start TTS - extract text from current page */
        zathura_document_t* document = zathura_get_document(controller->zathura);
        if (document == NULL) {
            girara_info("🚨 DEBUG: sc_tts_toggle - document is NULL!");
            tts_ui_controller_show_status(controller, "TTS: No document loaded", 2000);
            return false;
        }
        girara_info("✅ DEBUG: sc_tts_toggle - document found, getting current page...");
        
        unsigned int current_page_number = zathura_document_get_current_page_number(document);
        girara_info("🔍 DEBUG: sc_tts_toggle - current page number: %u", current_page_number);
        
        zathura_page_t* page = zathura_document_get_page(document, current_page_number);
        if (page == NULL) {
            girara_info("🚨 DEBUG: sc_tts_toggle - page is NULL!");
            tts_ui_controller_show_status(controller, "TTS: Cannot access current page", 2000);
            return false;
        }
        girara_info("✅ DEBUG: sc_tts_toggle - page found, extracting text...");
        
        /* Extract text segments from current page onwards (multi-page reading) */
        zathura_error_t error = ZATHURA_ERROR_OK;
        girara_list_t* segments = girara_list_new();
        girara_list_set_free_function(segments, (girara_free_function_t)tts_text_segment_free);
        
        unsigned int total_pages = zathura_document_get_number_of_pages(document);
        unsigned int pages_to_read = 3; /* Read current page + next 2 pages (configurable) */
        
        girara_info("🔍 DEBUG: sc_tts_toggle - extracting text from %u pages starting from page %u (total pages: %u)", 
                    pages_to_read, current_page_number, total_pages);
        
        for (unsigned int i = 0; i < pages_to_read && (current_page_number + i) < total_pages; i++) {
            unsigned int page_num = current_page_number + i;
            zathura_page_t* current_page = zathura_document_get_page(document, page_num);
            
            if (current_page != NULL) {
                girara_list_t* page_segments = tts_extract_text_segments(current_page, &error);
                if (page_segments != NULL && girara_list_size(page_segments) > 0) {
                    /* Copy all segments from this page to the main list */
                    for (size_t j = 0; j < girara_list_size(page_segments); j++) {
                        tts_text_segment_t* original_segment = girara_list_nth(page_segments, j);
                        if (original_segment != NULL && original_segment->text != NULL) {
                            /* Create a new segment copy to avoid double-free issues */
                            tts_text_segment_t* new_segment = tts_text_segment_new(
                                original_segment->text,
                                original_segment->bounds,
                                original_segment->page_number,
                                original_segment->segment_id,
                                original_segment->type
                            );
                            if (new_segment != NULL) {
                                girara_list_append(segments, new_segment);
                            }
                        }
                    }
                    girara_info("✅ DEBUG: sc_tts_toggle - added %zu segments from page %u", 
                                girara_list_size(page_segments), page_num);
                    
                    /* Free the page segments normally */
                    girara_list_free(page_segments);
                } else {
                    girara_info("🔍 DEBUG: sc_tts_toggle - no text found on page %u", page_num);
                    if (page_segments != NULL) {
                        girara_list_free(page_segments);
                    }
                }
            }
        }
        
        girara_info("🔍 DEBUG: sc_tts_toggle - text extraction result: segments=%p, total_segments=%zu, error=%d", 
                    (void*)segments, girara_list_size(segments), error);
        
        if (segments == NULL || girara_list_size(segments) == 0) {
            girara_info("🚨 DEBUG: sc_tts_toggle - no text segments found across %u pages", pages_to_read);
            tts_ui_controller_show_status(controller, "TTS: No readable text found", 2000);
            if (segments != NULL) {
                girara_list_free(segments);
            }
            return false;
        }
        girara_info("✅ DEBUG: sc_tts_toggle - found %zu text segments, starting audio session...", 
                    girara_list_size(segments));
        
        /* Start TTS session */
        if (tts_audio_controller_start_session(controller->audio_controller, segments)) {
            controller->tts_active = true;
            girara_info("✅ DEBUG: sc_tts_toggle - audio session started successfully");
            tts_ui_controller_show_status(controller, "TTS: Started reading", 2000);
        } else {
            girara_info("🚨 DEBUG: sc_tts_toggle - failed to start audio session");
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
    
    girara_info("🎯 DEBUG: sc_tts_pause_resume called - Ctrl+R pressed!");
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        girara_debug("🚨 DEBUG: sc_tts_pause_resume - controller or audio_controller is NULL");
        return false;
    }
    
    girara_debug("✅ DEBUG: sc_tts_pause_resume - controller found, checking current state...");
    
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
    
    /* This would need to be implemented in the audio controller */
    tts_ui_controller_show_status(controller, "TTS: Next segment (not implemented)", 2000);
    
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
    
    /* This would need to be implemented in the audio controller */
    tts_ui_controller_show_status(controller, "TTS: Previous segment (not implemented)", 2000);
    
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

/* Shortcut callback implementations */



/* Visual feedback functions */

void 
tts_ui_controller_update_progress(tts_ui_controller_t* controller, int current_segment, int total_segments) 
{
    if (controller == NULL || total_segments <= 0) {
        return;
    }
    
    /* Calculate progress percentage */
    float progress = (float)(current_segment + 1) / (float)total_segments * 100.0f;
    
    /* Create progress status message */
    char* progress_msg = g_strdup_printf("TTS: Reading segment %d/%d (%.1f%%)", 
                                        current_segment + 1, total_segments, progress);
    
    tts_ui_controller_show_status(controller, progress_msg, 1500);
    g_free(progress_msg);
}

void 
tts_ui_controller_show_tts_indicator(tts_ui_controller_t* controller, bool active) 
{
    if (controller == NULL || controller->session == NULL) {
        return;
    }
    
    /* Update TTS active state */
    controller->tts_active = active;
    
    /* Show indicator in status bar */
    if (active) {
        tts_ui_controller_show_status(controller, "TTS: ♪ Active", 0); /* No timeout for persistent indicator */
    } else {
        tts_ui_controller_clear_status(controller);
    }
}

bool 
tts_ui_controller_highlight_current_text(tts_ui_controller_t* controller, const char* text) 
{
    if (controller == NULL || text == NULL || controller->zathura == NULL) {
        return false;
    }
    
    /* Note: This is a simplified implementation. Full text highlighting would require
     * deeper integration with Zathura's page rendering system, similar to how search
     * results are highlighted. For now, we provide visual feedback through status messages.
     */
    
    /* Get current document and page */
    zathura_document_t* document = zathura_get_document(controller->zathura);
    if (document == NULL) {
        return false;
    }
    
    unsigned int current_page_number = zathura_document_get_current_page_number(document);
    zathura_page_t* page = zathura_document_get_page(document, current_page_number);
    if (page == NULL) {
        return false;
    }
    
    /* For now, show the current text being read in the status bar */
    /* This provides immediate visual feedback about what's being spoken */
    size_t text_len = strlen(text);
    char* display_text;
    
    if (text_len > 50) {
        /* Truncate long text for display */
        display_text = g_strdup_printf("TTS: \"%.47s...\"", text);
    } else {
        display_text = g_strdup_printf("TTS: \"%s\"", text);
    }
    
    tts_ui_controller_show_status(controller, display_text, 3000);
    g_free(display_text);
    
    /* TODO: Implement actual text highlighting on the page
     * This would require:
     * 1. Finding the text position on the page using text extraction
     * 2. Creating highlight rectangles similar to search results
     * 3. Setting page widget properties to draw the highlights
     * 4. Using a different color scheme for TTS highlights vs search highlights
     */
    
    return true;
}

/* Enhanced status display with TTS-specific formatting */

static void
tts_ui_controller_show_enhanced_status(tts_ui_controller_t* controller, 
                                      const char* action, 
                                      const char* details, 
                                      int timeout_ms)
{
    if (controller == NULL || action == NULL) {
        return;
    }
    
    char* status_msg;
    if (details != NULL) {
        status_msg = g_strdup_printf("TTS %s: %s", action, details);
    } else {
        status_msg = g_strdup_printf("TTS %s", action);
    }
    
    tts_ui_controller_show_status(controller, status_msg, timeout_ms);
    g_free(status_msg);
}

/* Progress tracking for long documents */

static void
tts_ui_controller_show_document_progress(tts_ui_controller_t* controller)
{
    if (controller == NULL || controller->audio_controller == NULL) {
        return;
    }
    
    int current_page = tts_audio_controller_get_current_page(controller->audio_controller);
    int current_segment = tts_audio_controller_get_current_segment(controller->audio_controller);
    
    if (current_page >= 0) {
        /* Get total pages from document */
        zathura_document_t* document = zathura_get_document(controller->zathura);
        if (document != NULL) {
            unsigned int total_pages = zathura_document_get_number_of_pages(document);
            
            char* progress_msg = g_strdup_printf("Page %d/%d, Segment %d", 
                                                current_page + 1, total_pages, current_segment + 1);
            tts_ui_controller_show_enhanced_status(controller, "Reading", progress_msg, 2000);
            g_free(progress_msg);
        }
    }
}

/* Visual state indicators */

static void
tts_ui_controller_update_visual_state(tts_ui_controller_t* controller, tts_audio_state_t state)
{
    if (controller == NULL) {
        return;
    }
    
    const char* state_icon;
    const char* state_text;
    
    switch (state) {
        case TTS_AUDIO_STATE_PLAYING:
            state_icon = "▶";
            state_text = "Playing";
            break;
        case TTS_AUDIO_STATE_PAUSED:
            state_icon = "⏸";
            state_text = "Paused";
            break;
        case TTS_AUDIO_STATE_STOPPED:
            state_icon = "⏹";
            state_text = "Stopped";
            break;
        case TTS_AUDIO_STATE_ERROR:
            state_icon = "⚠";
            state_text = "Error";
            break;
        default:
            state_icon = "?";
            state_text = "Unknown";
            break;
    }
    
    char* status_msg = g_strdup_printf("TTS %s %s", state_icon, state_text);
    
    /* Show persistent indicator for active states, temporary for stopped/error */
    int timeout = (state == TTS_AUDIO_STATE_STOPPED || state == TTS_AUDIO_STATE_ERROR) ? 2000 : 0;
    tts_ui_controller_show_status(controller, status_msg, timeout);
    
    g_free(status_msg);
}

/* Callback for audio state changes to update visual feedback */

static void
tts_audio_state_change_callback(tts_audio_state_t old_state, tts_audio_state_t new_state, void* user_data)
{
    tts_ui_controller_t* controller = (tts_ui_controller_t*)user_data;
    if (controller == NULL) {
        return;
    }
    
    /* Update visual state indicator */
    tts_ui_controller_update_visual_state(controller, new_state);
    
    /* Show document progress for active states */
    if (new_state == TTS_AUDIO_STATE_PLAYING) {
        tts_ui_controller_show_document_progress(controller);
    }
    
    /* Update TTS active indicator */
    bool is_active = (new_state == TTS_AUDIO_STATE_PLAYING || new_state == TTS_AUDIO_STATE_PAUSED);
    controller->tts_active = is_active;
}

/* Initialize visual feedback system */

bool
tts_ui_controller_init_visual_feedback(tts_ui_controller_t* controller)
{
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    /* Set up audio state change callback for visual feedback */
    tts_audio_controller_set_state_change_callback(controller->audio_controller,
                                                   tts_audio_state_change_callback,
                                                   controller);
    
    return true;
}

/* Command interface functions */

bool 
tts_ui_controller_register_commands(tts_ui_controller_t* controller) 
{
    if (controller == NULL || controller->session == NULL) {
        return false;
    }
    
    /* Register TTS commands with Girara */
    bool all_registered = true;
    
    all_registered &= girara_inputbar_command_add(controller->session, "tts", NULL, cmd_tts_toggle, NULL, "Toggle TTS on/off");
    all_registered &= girara_inputbar_command_add(controller->session, "tts-stop", NULL, cmd_tts_stop, NULL, "Stop TTS playback");
    all_registered &= girara_inputbar_command_add(controller->session, "tts-speed", NULL, cmd_tts_speed, NULL, "Set TTS speed (0.5-3.0)");
    all_registered &= girara_inputbar_command_add(controller->session, "tts-volume", NULL, cmd_tts_volume, NULL, "Set TTS volume (0-100)");
    all_registered &= girara_inputbar_command_add(controller->session, "tts-voice", NULL, cmd_tts_voice, NULL, "Set TTS voice");
    all_registered &= girara_inputbar_command_add(controller->session, "tts-engine", NULL, cmd_tts_engine, NULL, "Set TTS engine");
    all_registered &= girara_inputbar_command_add(controller->session, "tts-config", NULL, cmd_tts_config, NULL, "Configure TTS settings");
    all_registered &= girara_inputbar_command_add(controller->session, "tts-status", NULL, cmd_tts_status, NULL, "Show TTS status");
    
    if (all_registered) {
        tts_ui_controller_show_status(controller, "TTS: Commands registered", 2000);
    } else {
        tts_ui_controller_show_status(controller, "TTS: Some commands failed to register", 3000);
    }
    
    return all_registered;
}

void 
tts_ui_controller_unregister_commands(tts_ui_controller_t* controller) 
{
    if (controller == NULL) {
        return;
    }
    
    /* Note: Girara doesn't provide a direct way to unregister commands,
     * so this is a placeholder for future implementation */
}

/* TTS command functions (following Zathura's pattern) */

bool 
cmd_tts_toggle(girara_session_t* session, girara_list_t* argument_list) 
{
    (void)argument_list;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL) {
        return false;
    }
    
    /* Use the same logic as the shortcut handler */
    return sc_tts_toggle(session, NULL, NULL, 0);
}

bool 
cmd_tts_stop(girara_session_t* session, girara_list_t* argument_list) 
{
    (void)argument_list;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL) {
        return false;
    }
    
    /* Use the same logic as the shortcut handler */
    return sc_tts_stop(session, NULL, NULL, 0);
}

bool 
cmd_tts_speed(girara_session_t* session, girara_list_t* argument_list) 
{
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    /* If no arguments, show current speed */
    if (argument_list == NULL || girara_list_size(argument_list) == 0) {
        float current_speed = tts_audio_controller_get_speed(controller->audio_controller);
        char* status_msg = g_strdup_printf("TTS: Current speed %.1fx", current_speed);
        tts_ui_controller_show_status(controller, status_msg, 3000);
        g_free(status_msg);
        return true;
    }
    
    /* Get speed argument */
    char* speed_str = girara_list_nth(argument_list, 0);
    if (speed_str == NULL) {
        tts_ui_controller_show_status(controller, "TTS: Invalid speed argument", 2000);
        return false;
    }
    
    float new_speed = g_ascii_strtod(speed_str, NULL);
    if (new_speed < 0.5f || new_speed > 3.0f) {
        tts_ui_controller_show_status(controller, "TTS: Speed must be between 0.5 and 3.0", 2000);
        return false;
    }
    
    if (tts_audio_controller_set_speed(controller->audio_controller, new_speed)) {
        char* status_msg = g_strdup_printf("TTS: Speed set to %.1fx", new_speed);
        tts_ui_controller_show_status(controller, status_msg, 2000);
        g_free(status_msg);
        return true;
    } else {
        tts_ui_controller_show_status(controller, "TTS: Failed to set speed", 2000);
        return false;
    }
}

bool 
cmd_tts_volume(girara_session_t* session, girara_list_t* argument_list) 
{
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    /* If no arguments, show current volume */
    if (argument_list == NULL || girara_list_size(argument_list) == 0) {
        int current_volume = tts_audio_controller_get_volume(controller->audio_controller);
        char* status_msg = g_strdup_printf("TTS: Current volume %d%%", current_volume);
        tts_ui_controller_show_status(controller, status_msg, 3000);
        g_free(status_msg);
        return true;
    }
    
    /* Get volume argument */
    char* volume_str = girara_list_nth(argument_list, 0);
    if (volume_str == NULL) {
        tts_ui_controller_show_status(controller, "TTS: Invalid volume argument", 2000);
        return false;
    }
    
    int new_volume = atoi(volume_str);
    if (new_volume < 0 || new_volume > 100) {
        tts_ui_controller_show_status(controller, "TTS: Volume must be between 0 and 100", 2000);
        return false;
    }
    
    if (tts_audio_controller_set_volume(controller->audio_controller, new_volume)) {
        char* status_msg = g_strdup_printf("TTS: Volume set to %d%%", new_volume);
        tts_ui_controller_show_status(controller, status_msg, 2000);
        g_free(status_msg);
        return true;
    } else {
        tts_ui_controller_show_status(controller, "TTS: Failed to set volume", 2000);
        return false;
    }
}

bool 
cmd_tts_voice(girara_session_t* session, girara_list_t* argument_list) 
{
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL) {
        return false;
    }
    
    /* If no arguments, list available voices */
    if (argument_list == NULL || girara_list_size(argument_list) == 0) {
        tts_ui_controller_show_status(controller, "TTS: Available voices: default (use 'tts-voice <name>' to select)", 4000);
        return true;
    }
    
    /* Get voice argument */
    char* voice_name = girara_list_nth(argument_list, 0);
    if (voice_name == NULL) {
        tts_ui_controller_show_status(controller, "TTS: Invalid voice argument", 2000);
        return false;
    }
    
    /* For now, just show that the voice would be set */
    char* status_msg = g_strdup_printf("TTS: Voice set to '%s' (restart TTS to apply)", voice_name);
    tts_ui_controller_show_status(controller, status_msg, 3000);
    g_free(status_msg);
    
    return true;
}

bool 
cmd_tts_engine(girara_session_t* session, girara_list_t* argument_list) 
{
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL) {
        return false;
    }
    
    /* If no arguments, show current engine and available engines */
    if (argument_list == NULL || girara_list_size(argument_list) == 0) {
        tts_ui_controller_show_status(controller, "TTS: Available engines: piper, speech_dispatcher, espeak", 4000);
        return true;
    }
    
    /* Get engine argument */
    char* engine_name = girara_list_nth(argument_list, 0);
    if (engine_name == NULL) {
        tts_ui_controller_show_status(controller, "TTS: Invalid engine argument", 2000);
        return false;
    }
    
    /* Validate engine name */
    if (g_strcmp0(engine_name, "piper") != 0 && 
        g_strcmp0(engine_name, "speech_dispatcher") != 0 && 
        g_strcmp0(engine_name, "espeak") != 0) {
        tts_ui_controller_show_status(controller, "TTS: Invalid engine. Use: piper, speech_dispatcher, or espeak", 3000);
        return false;
    }
    
    /* For now, just show that the engine would be set */
    char* status_msg = g_strdup_printf("TTS: Engine set to '%s' (restart TTS to apply)", engine_name);
    tts_ui_controller_show_status(controller, status_msg, 3000);
    g_free(status_msg);
    
    return true;
}

bool 
cmd_tts_config(girara_session_t* session, girara_list_t* argument_list) 
{
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL) {
        return false;
    }
    
    /* If no arguments, show configuration help */
    if (argument_list == NULL || girara_list_size(argument_list) == 0) {
        tts_ui_controller_show_status(controller, "TTS: Config options: speed, volume, engine, voice. Use 'tts-config <option> <value>'", 5000);
        return true;
    }
    
    /* Get config option */
    char* option = girara_list_nth(argument_list, 0);
    if (option == NULL) {
        tts_ui_controller_show_status(controller, "TTS: Invalid config option", 2000);
        return false;
    }
    
    /* Handle different config options */
    if (g_strcmp0(option, "speed") == 0) {
        if (girara_list_size(argument_list) < 2) {
            return cmd_tts_speed(session, NULL); /* Show current speed */
        } else {
            girara_list_t* speed_args = girara_list_new();
            girara_list_append(speed_args, girara_list_nth(argument_list, 1));
            bool result = cmd_tts_speed(session, speed_args);
            girara_list_free(speed_args);
            return result;
        }
    } else if (g_strcmp0(option, "volume") == 0) {
        if (girara_list_size(argument_list) < 2) {
            return cmd_tts_volume(session, NULL); /* Show current volume */
        } else {
            girara_list_t* volume_args = girara_list_new();
            girara_list_append(volume_args, girara_list_nth(argument_list, 1));
            bool result = cmd_tts_volume(session, volume_args);
            girara_list_free(volume_args);
            return result;
        }
    } else if (g_strcmp0(option, "engine") == 0) {
        if (girara_list_size(argument_list) < 2) {
            return cmd_tts_engine(session, NULL); /* Show available engines */
        } else {
            girara_list_t* engine_args = girara_list_new();
            girara_list_append(engine_args, girara_list_nth(argument_list, 1));
            bool result = cmd_tts_engine(session, engine_args);
            girara_list_free(engine_args);
            return result;
        }
    } else if (g_strcmp0(option, "voice") == 0) {
        if (girara_list_size(argument_list) < 2) {
            return cmd_tts_voice(session, NULL); /* Show available voices */
        } else {
            girara_list_t* voice_args = girara_list_new();
            girara_list_append(voice_args, girara_list_nth(argument_list, 1));
            bool result = cmd_tts_voice(session, voice_args);
            girara_list_free(voice_args);
            return result;
        }
    } else {
        tts_ui_controller_show_status(controller, "TTS: Unknown config option. Use: speed, volume, engine, voice", 3000);
        return false;
    }
}

bool 
cmd_tts_status(girara_session_t* session, girara_list_t* argument_list) 
{
    (void)argument_list;
    
    tts_ui_controller_t* controller = tts_ui_controller_get_from_session(session);
    if (controller == NULL) {
        return false;
    }
    
    /* Use the same logic as the shortcut handler */
    return sc_tts_settings(session, NULL, NULL, 0);
}

/* Streaming command removed - streaming is now the only mode */

/* 
User notification and status update functions */

/* Error callback for TTS error handling integration */
static void
tts_ui_error_callback(const tts_error_context_t* error_context, void* user_data)
{
    tts_ui_controller_t* controller = (tts_ui_controller_t*)user_data;
    if (controller == NULL || error_context == NULL) {
        return;
    }
    
    /* Get user-friendly error message */
    char* user_message = tts_error_get_user_message(error_context->error_code, error_context->details);
    if (user_message == NULL) {
        return;
    }
    
    /* Determine timeout based on severity */
    int timeout_ms = 3000; /* Default timeout */
    switch (error_context->severity) {
        case TTS_ERROR_SEVERITY_INFO:
            timeout_ms = 2000;
            break;
        case TTS_ERROR_SEVERITY_WARNING:
            timeout_ms = 4000;
            break;
        case TTS_ERROR_SEVERITY_ERROR:
            timeout_ms = 5000;
            break;
        case TTS_ERROR_SEVERITY_CRITICAL:
            timeout_ms = 8000;
            break;
    }
    
    /* Show error notification */
    tts_ui_controller_show_status(controller, user_message, timeout_ms);
    
    g_free(user_message);
}

/* Initialize error handling integration */
bool
tts_ui_controller_init_error_handling(tts_ui_controller_t* controller)
{
    if (controller == NULL) {
        return false;
    }
    
    /* Set up error callback for user notifications */
    tts_error_set_callback(tts_ui_error_callback, controller);
    
    return true;
}

/* Engine switching notifications */
void
tts_ui_controller_notify_engine_switch(tts_ui_controller_t* controller, 
                                       const char* from_engine, 
                                       const char* to_engine,
                                       const char* reason)
{
    if (controller == NULL) {
        return;
    }
    
    char* notification;
    if (from_engine != NULL && to_engine != NULL) {
        if (reason != NULL) {
            notification = g_strdup_printf("TTS: Switched from %s to %s (%s)", 
                                         from_engine, to_engine, reason);
        } else {
            notification = g_strdup_printf("TTS: Switched from %s to %s", 
                                         from_engine, to_engine);
        }
    } else if (to_engine != NULL) {
        notification = g_strdup_printf("TTS: Using %s engine", to_engine);
    } else {
        notification = g_strdup("TTS: Engine switched");
    }
    
    tts_ui_controller_show_status(controller, notification, 4000);
    g_free(notification);
}

/* Content availability notifications */
void
tts_ui_controller_notify_content_unavailable(tts_ui_controller_t* controller, 
                                             const char* content_type,
                                             const char* suggestion)
{
    if (controller == NULL) {
        return;
    }
    
    char* notification;
    if (content_type != NULL && suggestion != NULL) {
        notification = g_strdup_printf("TTS: %s not available. %s", content_type, suggestion);
    } else if (content_type != NULL) {
        notification = g_strdup_printf("TTS: %s not available", content_type);
    } else {
        notification = g_strdup("TTS: Content not available for reading");
    }
    
    tts_ui_controller_show_status(controller, notification, 4000);
    g_free(notification);
}

/* Feature availability notifications */
void
tts_ui_controller_notify_feature_unavailable(tts_ui_controller_t* controller,
                                             const char* feature_name,
                                             const char* reason)
{
    if (controller == NULL) {
        return;
    }
    
    char* notification;
    if (feature_name != NULL && reason != NULL) {
        notification = g_strdup_printf("TTS: %s unavailable - %s", feature_name, reason);
    } else if (feature_name != NULL) {
        notification = g_strdup_printf("TTS: %s is not available", feature_name);
    } else {
        notification = g_strdup("TTS: Feature not available");
    }
    
    tts_ui_controller_show_status(controller, notification, 3000);
    g_free(notification);
}

/* State change notifications with enhanced feedback */
void
tts_ui_controller_notify_state_change(tts_ui_controller_t* controller,
                                      tts_audio_state_t old_state,
                                      tts_audio_state_t new_state,
                                      const char* additional_info)
{
    if (controller == NULL) {
        return;
    }
    
    const char* state_icon = "?";
    const char* state_text = "Unknown";
    const char* state_color = ""; /* Could be used for colored output in future */
    
    switch (new_state) {
        case TTS_AUDIO_STATE_PLAYING:
            state_icon = "▶";
            state_text = "Playing";
            state_color = "green";
            break;
        case TTS_AUDIO_STATE_PAUSED:
            state_icon = "⏸";
            state_text = "Paused";
            state_color = "yellow";
            break;
        case TTS_AUDIO_STATE_STOPPED:
            state_icon = "⏹";
            state_text = "Stopped";
            state_color = "red";
            break;
        case TTS_AUDIO_STATE_ERROR:
            state_icon = "⚠";
            state_text = "Error";
            state_color = "red";
            break;
    }
    
    char* notification;
    if (additional_info != NULL) {
        notification = g_strdup_printf("TTS %s %s - %s", state_icon, state_text, additional_info);
    } else {
        notification = g_strdup_printf("TTS %s %s", state_icon, state_text);
    }
    
    /* Determine timeout based on state */
    int timeout_ms = 2000;
    if (new_state == TTS_AUDIO_STATE_ERROR) {
        timeout_ms = 5000;
    } else if (new_state == TTS_AUDIO_STATE_PLAYING) {
        timeout_ms = 1500; /* Shorter for active states */
    }
    
    tts_ui_controller_show_status(controller, notification, timeout_ms);
    g_free(notification);
}

/* Progress notifications with detailed information */
void
tts_ui_controller_notify_progress_detailed(tts_ui_controller_t* controller,
                                           int current_page,
                                           int total_pages,
                                           int current_segment,
                                           int total_segments,
                                           const char* current_text_preview)
{
    if (controller == NULL) {
        return;
    }
    
    char* notification;
    
    if (current_text_preview != NULL && strlen(current_text_preview) > 0) {
        /* Show text preview with progress */
        size_t preview_len = strlen(current_text_preview);
        if (preview_len > 30) {
            notification = g_strdup_printf("TTS: Page %d/%d, Segment %d/%d - \"%.27s...\"",
                                         current_page + 1, total_pages,
                                         current_segment + 1, total_segments,
                                         current_text_preview);
        } else {
            notification = g_strdup_printf("TTS: Page %d/%d, Segment %d/%d - \"%s\"",
                                         current_page + 1, total_pages,
                                         current_segment + 1, total_segments,
                                         current_text_preview);
        }
    } else {
        /* Show just progress information */
        notification = g_strdup_printf("TTS: Page %d/%d, Segment %d/%d",
                                     current_page + 1, total_pages,
                                     current_segment + 1, total_segments);
    }
    
    tts_ui_controller_show_status(controller, notification, 2000);
    g_free(notification);
}

/* Configuration change notifications */
void
tts_ui_controller_notify_config_change(tts_ui_controller_t* controller,
                                       const char* setting_name,
                                       const char* old_value,
                                       const char* new_value)
{
    if (controller == NULL || setting_name == NULL) {
        return;
    }
    
    char* notification;
    if (old_value != NULL && new_value != NULL) {
        notification = g_strdup_printf("TTS: %s changed from %s to %s",
                                     setting_name, old_value, new_value);
    } else if (new_value != NULL) {
        notification = g_strdup_printf("TTS: %s set to %s", setting_name, new_value);
    } else {
        notification = g_strdup_printf("TTS: %s updated", setting_name);
    }
    
    tts_ui_controller_show_status(controller, notification, 3000);
    g_free(notification);
}

/* System status notifications */
void
tts_ui_controller_notify_system_status(tts_ui_controller_t* controller,
                                       const char* component,
                                       const char* status,
                                       bool is_error)
{
    if (controller == NULL) {
        return;
    }
    
    char* notification;
    const char* prefix = is_error ? "⚠ TTS" : "TTS";
    
    if (component != NULL && status != NULL) {
        notification = g_strdup_printf("%s: %s - %s", prefix, component, status);
    } else if (status != NULL) {
        notification = g_strdup_printf("%s: %s", prefix, status);
    } else {
        notification = g_strdup_printf("%s: System status update", prefix);
    }
    
    int timeout_ms = is_error ? 5000 : 3000;
    tts_ui_controller_show_status(controller, notification, timeout_ms);
    g_free(notification);
}

/* Enhanced callback for audio state changes with notifications */
static void
tts_audio_state_change_callback_enhanced(tts_audio_state_t old_state, 
                                         tts_audio_state_t new_state, 
                                         void* user_data)
{
    tts_ui_controller_t* controller = (tts_ui_controller_t*)user_data;
    if (controller == NULL) {
        return;
    }
    
    /* Update visual state indicator */
    tts_ui_controller_update_visual_state(controller, new_state);
    
    /* Provide detailed state change notification */
    const char* additional_info = NULL;
    
    /* Add context-specific information */
    if (new_state == TTS_AUDIO_STATE_PLAYING && old_state == TTS_AUDIO_STATE_STOPPED) {
        additional_info = "Started reading";
    } else if (new_state == TTS_AUDIO_STATE_PAUSED && old_state == TTS_AUDIO_STATE_PLAYING) {
        additional_info = "Playback paused";
    } else if (new_state == TTS_AUDIO_STATE_PLAYING && old_state == TTS_AUDIO_STATE_PAUSED) {
        additional_info = "Playback resumed";
    } else if (new_state == TTS_AUDIO_STATE_STOPPED) {
        additional_info = "Playback stopped";
    } else if (new_state == TTS_AUDIO_STATE_ERROR) {
        additional_info = "An error occurred";
    }
    
    /* Send state change notification */
    tts_ui_controller_notify_state_change(controller, old_state, new_state, additional_info);
    
    /* Show document progress for active states */
    if (new_state == TTS_AUDIO_STATE_PLAYING) {
        tts_ui_controller_show_document_progress(controller);
    }
    
    /* Update TTS active indicator */
    bool is_active = (new_state == TTS_AUDIO_STATE_PLAYING || new_state == TTS_AUDIO_STATE_PAUSED);
    controller->tts_active = is_active;
}

/* Enhanced visual feedback initialization with notifications */
bool
tts_ui_controller_init_notifications(tts_ui_controller_t* controller)
{
    if (controller == NULL || controller->audio_controller == NULL) {
        return false;
    }
    
    /* Set up enhanced audio state change callback */
    tts_audio_controller_set_state_change_callback(controller->audio_controller,
                                                   tts_audio_state_change_callback_enhanced,
                                                   controller);
    
    /* Initialize error handling integration */
    tts_ui_controller_init_error_handling(controller);
    
    /* Show initialization notification */
    tts_ui_controller_notify_system_status(controller, "Notifications", "Initialized", false);
    
    return true;
}