/* TTS Audio Controller Implementation
 * Manages TTS playback state and audio session management
 */

#include "tts-audio-controller.h"
#include <girara/utils.h>
#include <girara/datastructures.h>
#include <string.h>

/* Audio controller management functions */

tts_audio_controller_t* 
tts_audio_controller_new(void) 
{
    tts_audio_controller_t* controller = g_malloc0(sizeof(tts_audio_controller_t));
    if (controller == NULL) {
        return NULL;
    }
    
    /* Initialize state */
    controller->state = TTS_AUDIO_STATE_STOPPED;
    g_mutex_init(&controller->state_mutex);
    g_cond_init(&controller->state_cond);
    
    /* Initialize position */
    controller->current_page = -1;
    controller->current_segment = -1;
    controller->text_segments = NULL;
    
    /* Initialize audio settings with defaults */
    controller->speed_multiplier = 1.0f;
    controller->volume_level = 80;
    
    /* Initialize threading */
    controller->audio_thread = NULL;
    controller->should_stop = false;
    
    /* Initialize text */
    controller->current_text = NULL;
    
    /* Initialize TTS engine */
    controller->tts_engine = NULL;
    
    /* Initialize callbacks */
    controller->state_change_callback = NULL;
    controller->callback_user_data = NULL;
    
    return controller;
}

void 
tts_audio_controller_free(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return;
    }
    
    /* Stop any active session */
    tts_audio_controller_stop_session(controller);
    
    /* Clean up text segments list */
    if (controller->text_segments != NULL) {
        girara_list_free(controller->text_segments);
    }
    
    /* Clean up current text */
    g_free(controller->current_text);
    
    /* Clean up synchronization primitives */
    g_mutex_clear(&controller->state_mutex);
    g_cond_clear(&controller->state_cond);
    
    g_free(controller);
}

/* State management functions */

tts_audio_state_t 
tts_audio_controller_get_state(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return TTS_AUDIO_STATE_ERROR;
    }
    
    tts_audio_state_t state;
    g_mutex_lock(&controller->state_mutex);
    state = controller->state;
    g_mutex_unlock(&controller->state_mutex);
    
    return state;
}

bool 
tts_audio_controller_set_state(tts_audio_controller_t* controller, tts_audio_state_t new_state) 
{
    if (controller == NULL) {
        return false;
    }
    
    g_mutex_lock(&controller->state_mutex);
    
    tts_audio_state_t old_state = controller->state;
    
    /* Validate state transition */
    bool valid_transition = false;
    switch (old_state) {
        case TTS_AUDIO_STATE_STOPPED:
            valid_transition = (new_state == TTS_AUDIO_STATE_PLAYING || 
                              new_state == TTS_AUDIO_STATE_ERROR);
            break;
        case TTS_AUDIO_STATE_PLAYING:
            valid_transition = (new_state == TTS_AUDIO_STATE_PAUSED || 
                              new_state == TTS_AUDIO_STATE_STOPPED ||
                              new_state == TTS_AUDIO_STATE_ERROR);
            break;
        case TTS_AUDIO_STATE_PAUSED:
            valid_transition = (new_state == TTS_AUDIO_STATE_PLAYING || 
                              new_state == TTS_AUDIO_STATE_STOPPED ||
                              new_state == TTS_AUDIO_STATE_ERROR);
            break;
        case TTS_AUDIO_STATE_ERROR:
            valid_transition = (new_state == TTS_AUDIO_STATE_STOPPED);
            break;
    }
    
    if (!valid_transition) {
        g_mutex_unlock(&controller->state_mutex);
        return false;
    }
    
    controller->state = new_state;
    
    /* Signal state change */
    g_cond_broadcast(&controller->state_cond);
    
    g_mutex_unlock(&controller->state_mutex);
    
    /* Call state change callback if set */
    if (controller->state_change_callback != NULL) {
        controller->state_change_callback(old_state, new_state, controller->callback_user_data);
    }
    
    return true;
}

/* Session management functions */

bool 
tts_audio_controller_start_session(tts_audio_controller_t* controller, girara_list_t* segments) 
{
    if (controller == NULL || segments == NULL) {
        return false;
    }
    
    g_mutex_lock(&controller->state_mutex);
    
    /* Can only start session if stopped */
    if (controller->state != TTS_AUDIO_STATE_STOPPED) {
        g_mutex_unlock(&controller->state_mutex);
        return false;
    }
    
    /* Clean up any existing segments */
    if (controller->text_segments != NULL) {
        girara_list_free(controller->text_segments);
    }
    
    /* Store reference to segments */
    controller->text_segments = segments;
    
    /* Reset position to beginning */
    controller->current_page = -1;
    controller->current_segment = -1;
    
    /* Find first segment */
    if (girara_list_size(segments) > 0) {
        tts_text_segment_t* first_segment = girara_list_nth(segments, 0);
        if (first_segment != NULL) {
            controller->current_page = first_segment->page_number;
            controller->current_segment = 0;
        }
    }
    
    /* Reset stop flag */
    controller->should_stop = false;
    
    g_mutex_unlock(&controller->state_mutex);
    
    /* Set state to playing */
    return tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_PLAYING);
}

void 
tts_audio_controller_stop_session(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return;
    }
    
    g_mutex_lock(&controller->state_mutex);
    
    /* Set stop flag for audio thread */
    controller->should_stop = true;
    
    /* Wait for audio thread to finish if it exists */
    if (controller->audio_thread != NULL) {
        g_mutex_unlock(&controller->state_mutex);
        g_thread_join(controller->audio_thread);
        g_mutex_lock(&controller->state_mutex);
        controller->audio_thread = NULL;
    }
    
    /* Reset position */
    controller->current_page = -1;
    controller->current_segment = -1;
    
    /* Clean up current text */
    g_free(controller->current_text);
    controller->current_text = NULL;
    
    g_mutex_unlock(&controller->state_mutex);
    
    /* Set state to stopped */
    tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_STOPPED);
}

bool 
tts_audio_controller_pause_session(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return false;
    }
    
    return tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_PAUSED);
}

bool 
tts_audio_controller_resume_session(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return false;
    }
    
    return tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_PLAYING);
}

/* Position management functions */

int 
tts_audio_controller_get_current_page(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return -1;
    }
    
    int page;
    g_mutex_lock(&controller->state_mutex);
    page = controller->current_page;
    g_mutex_unlock(&controller->state_mutex);
    
    return page;
}

int 
tts_audio_controller_get_current_segment(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return -1;
    }
    
    int segment;
    g_mutex_lock(&controller->state_mutex);
    segment = controller->current_segment;
    g_mutex_unlock(&controller->state_mutex);
    
    return segment;
}

bool 
tts_audio_controller_set_position(tts_audio_controller_t* controller, int page, int segment) 
{
    if (controller == NULL || controller->text_segments == NULL) {
        return false;
    }
    
    g_mutex_lock(&controller->state_mutex);
    
    /* Validate segment index */
    size_t segment_count = girara_list_size(controller->text_segments);
    if (segment < 0 || (size_t)segment >= segment_count) {
        g_mutex_unlock(&controller->state_mutex);
        return false;
    }
    
    /* Validate that segment belongs to the specified page */
    tts_text_segment_t* target_segment = girara_list_nth(controller->text_segments, segment);
    if (target_segment == NULL || target_segment->page_number != page) {
        g_mutex_unlock(&controller->state_mutex);
        return false;
    }
    
    controller->current_page = page;
    controller->current_segment = segment;
    
    g_mutex_unlock(&controller->state_mutex);
    
    return true;
}

/* Audio settings functions */

float 
tts_audio_controller_get_speed(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return 1.0f;
    }
    
    float speed;
    g_mutex_lock(&controller->state_mutex);
    speed = controller->speed_multiplier;
    g_mutex_unlock(&controller->state_mutex);
    
    return speed;
}

bool 
tts_audio_controller_set_speed(tts_audio_controller_t* controller, float speed) 
{
    if (controller == NULL || speed <= 0.0f || speed > 5.0f) {
        return false;
    }
    
    g_mutex_lock(&controller->state_mutex);
    controller->speed_multiplier = speed;
    g_mutex_unlock(&controller->state_mutex);
    
    return true;
}

int 
tts_audio_controller_get_volume(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return 80;
    }
    
    int volume;
    g_mutex_lock(&controller->state_mutex);
    volume = controller->volume_level;
    g_mutex_unlock(&controller->state_mutex);
    
    return volume;
}

bool 
tts_audio_controller_set_volume(tts_audio_controller_t* controller, int volume) 
{
    if (controller == NULL || volume < 0 || volume > 100) {
        return false;
    }
    
    g_mutex_lock(&controller->state_mutex);
    controller->volume_level = volume;
    g_mutex_unlock(&controller->state_mutex);
    
    return true;
}

/* Thread synchronization functions */

void 
tts_audio_controller_lock(tts_audio_controller_t* controller) 
{
    if (controller != NULL) {
        g_mutex_lock(&controller->state_mutex);
    }
}

void 
tts_audio_controller_unlock(tts_audio_controller_t* controller) 
{
    if (controller != NULL) {
        g_mutex_unlock(&controller->state_mutex);
    }
}

void 
tts_audio_controller_wait_for_state_change(tts_audio_controller_t* controller) 
{
    if (controller != NULL) {
        g_cond_wait(&controller->state_cond, &controller->state_mutex);
    }
}

void 
tts_audio_controller_signal_state_change(tts_audio_controller_t* controller) 
{
    if (controller != NULL) {
        g_cond_broadcast(&controller->state_cond);
    }
}

/* Callback management */

void 
tts_audio_controller_set_state_change_callback(tts_audio_controller_t* controller,
                                               void (*callback)(tts_audio_state_t, tts_audio_state_t, void*),
                                               void* user_data) 
{
    if (controller == NULL) {
        return;
    }
    
    g_mutex_lock(&controller->state_mutex);
    controller->state_change_callback = callback;
    controller->callback_user_data = user_data;
    g_mutex_unlock(&controller->state_mutex);
}

/* Text segment helper functions */

tts_text_segment_t* 
tts_text_segment_new(const char* text, int page, int segment_id) 
{
    if (text == NULL) {
        return NULL;
    }
    
    tts_text_segment_t* segment = g_malloc0(sizeof(tts_text_segment_t));
    if (segment == NULL) {
        return NULL;
    }
    
    segment->text = g_strdup(text);
    segment->page_number = page;
    segment->segment_id = segment_id;
    segment->start_offset = 0;
    segment->end_offset = strlen(text);
    
    return segment;
}

void 
tts_text_segment_free(tts_text_segment_t* segment) 
{
    if (segment == NULL) {
        return;
    }
    
    g_free(segment->text);
    g_free(segment);
}/* P
layback control functions */

bool 
tts_audio_controller_play_text(tts_audio_controller_t* controller, const char* text) 
{
    if (controller == NULL || text == NULL || controller->tts_engine == NULL) {
        return false;
    }
    
    /* Include TTS engine header for function calls */
    #include "tts-engine.h"
    
    tts_engine_t* engine = (tts_engine_t*)controller->tts_engine;
    zathura_error_t error;
    
    g_mutex_lock(&controller->state_mutex);
    
    /* Update current text */
    g_free(controller->current_text);
    controller->current_text = g_strdup(text);
    
    /* Apply current speed and volume settings to engine */
    tts_engine_config_t* config = tts_engine_config_new();
    if (config != NULL) {
        config->speed = controller->speed_multiplier;
        config->volume = controller->volume_level;
        config->voice_name = NULL; /* Use current voice */
        config->pitch = 0;
        
        tts_engine_set_config(engine, config, &error);
        tts_engine_config_free(config);
    }
    
    g_mutex_unlock(&controller->state_mutex);
    
    /* Start speaking the text */
    bool success = tts_engine_speak(engine, text, &error);
    
    if (success) {
        tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_PLAYING);
    } else {
        tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_ERROR);
    }
    
    return success;
}

bool 
tts_audio_controller_play_current_segment(tts_audio_controller_t* controller) 
{
    if (controller == NULL || controller->text_segments == NULL) {
        return false;
    }
    
    g_mutex_lock(&controller->state_mutex);
    
    /* Get current segment */
    if (controller->current_segment < 0 || 
        (size_t)controller->current_segment >= girara_list_size(controller->text_segments)) {
        g_mutex_unlock(&controller->state_mutex);
        return false;
    }
    
    tts_text_segment_t* segment = girara_list_nth(controller->text_segments, controller->current_segment);
    if (segment == NULL || segment->text == NULL) {
        g_mutex_unlock(&controller->state_mutex);
        return false;
    }
    
    char* text_to_speak = g_strdup(segment->text);
    g_mutex_unlock(&controller->state_mutex);
    
    /* Play the segment text */
    bool success = tts_audio_controller_play_text(controller, text_to_speak);
    g_free(text_to_speak);
    
    return success;
}

bool 
tts_audio_controller_navigate_to_segment(tts_audio_controller_t* controller, int direction) 
{
    if (controller == NULL || controller->text_segments == NULL) {
        return false;
    }
    
    g_mutex_lock(&controller->state_mutex);
    
    size_t segment_count = girara_list_size(controller->text_segments);
    if (segment_count == 0) {
        g_mutex_unlock(&controller->state_mutex);
        return false;
    }
    
    int new_segment = controller->current_segment + direction;
    
    /* Validate new segment index */
    if (new_segment < 0 || (size_t)new_segment >= segment_count) {
        g_mutex_unlock(&controller->state_mutex);
        return false;
    }
    
    /* Get the target segment to update page number */
    tts_text_segment_t* target_segment = girara_list_nth(controller->text_segments, new_segment);
    if (target_segment == NULL) {
        g_mutex_unlock(&controller->state_mutex);
        return false;
    }
    
    /* Update position */
    controller->current_segment = new_segment;
    controller->current_page = target_segment->page_number;
    
    g_mutex_unlock(&controller->state_mutex);
    
    /* If currently playing, start playing the new segment */
    if (tts_audio_controller_get_state(controller) == TTS_AUDIO_STATE_PLAYING) {
        return tts_audio_controller_play_current_segment(controller);
    }
    
    return true;
}

bool 
tts_audio_controller_navigate_to_page(tts_audio_controller_t* controller, int page) 
{
    if (controller == NULL || controller->text_segments == NULL || page < 0) {
        return false;
    }
    
    g_mutex_lock(&controller->state_mutex);
    
    size_t segment_count = girara_list_size(controller->text_segments);
    
    /* Find first segment on the specified page */
    int target_segment = -1;
    for (size_t i = 0; i < segment_count; i++) {
        tts_text_segment_t* segment = girara_list_nth(controller->text_segments, i);
        if (segment != NULL && segment->page_number == page) {
            target_segment = (int)i;
            break;
        }
    }
    
    if (target_segment == -1) {
        g_mutex_unlock(&controller->state_mutex);
        return false; /* No segments found on this page */
    }
    
    /* Update position */
    controller->current_segment = target_segment;
    controller->current_page = page;
    
    g_mutex_unlock(&controller->state_mutex);
    
    /* If currently playing, start playing the new segment */
    if (tts_audio_controller_get_state(controller) == TTS_AUDIO_STATE_PLAYING) {
        return tts_audio_controller_play_current_segment(controller);
    }
    
    return true;
}

/* Engine integration */

void 
tts_audio_controller_set_engine(tts_audio_controller_t* controller, void* engine) 
{
    if (controller == NULL) {
        return;
    }
    
    g_mutex_lock(&controller->state_mutex);
    controller->tts_engine = engine;
    g_mutex_unlock(&controller->state_mutex);
}

void* 
tts_audio_controller_get_engine(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return NULL;
    }
    
    void* engine;
    g_mutex_lock(&controller->state_mutex);
    engine = controller->tts_engine;
    g_mutex_unlock(&controller->state_mutex);
    
    return engine;
}