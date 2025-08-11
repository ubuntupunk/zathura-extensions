/* TTS Audio Controller Implementation
 * Manages TTS playback state and audio session management
 */

#define _DEFAULT_SOURCE
#include "tts-audio-controller.h"
#include "tts-engine.h"
#include "tts-streaming-engine.h"
#include <girara/utils.h>
#include <girara/datastructures.h>
#include <girara/log.h>
#include <string.h>
#include <unistd.h>

/* Forward declarations */
static bool tts_audio_controller_start_streaming_session(tts_audio_controller_t* controller, girara_list_t* segments);
static void tts_audio_controller_stop_streaming_session(tts_audio_controller_t* controller);

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
    
    /* Initialize streaming engine */
    controller->streaming_engine = NULL;
    
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
    
    /* Current text cleanup removed - handled by streaming engine */
    
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
    
    /* Stop any existing session first */
    if (controller->state != TTS_AUDIO_STATE_STOPPED) {
        girara_info("🔧 DEBUG: start_session - stopping existing session first");
        g_mutex_unlock(&controller->state_mutex);
        tts_audio_controller_stop_session(controller);
        g_mutex_lock(&controller->state_mutex);
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
    
    /* Stop flag removed - handled by streaming engine */
    
    /* Streaming is always enabled now */
    
    g_mutex_unlock(&controller->state_mutex);
    
    /* Set state to playing */
    if (!tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_PLAYING)) {
        return false;
    }
    
    /* Always use streaming engine for seamless playback */
    girara_info("🚀 DEBUG: Using streaming TTS engine for session");
    girara_info("🔧 DEBUG: About to call start_streaming_session with %zu segments", girara_list_size(segments));
    
    bool streaming_result = tts_audio_controller_start_streaming_session(controller, segments);
    girara_info("🔧 DEBUG: start_streaming_session returned: %s", streaming_result ? "SUCCESS" : "FAILED");
    
    if (!streaming_result) {
        girara_error("Failed to start streaming TTS session");
        tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_ERROR);
        return false;
    }
    
    return true;
}

void 
tts_audio_controller_stop_session(tts_audio_controller_t* controller) 
{
    if (controller == NULL) {
        return;
    }
    
    g_mutex_lock(&controller->state_mutex);
    
    /* Reset position */
    controller->current_page = -1;
    controller->current_segment = -1;
    
    g_mutex_unlock(&controller->state_mutex);
    
    /* Stop streaming session */
    tts_audio_controller_stop_streaming_session(controller);
    
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

/* Text segment helper functions are now defined in tts-text-extractor.c *//* P
layback control functions */

bool 
tts_audio_controller_play_text(tts_audio_controller_t* controller, const char* text) 
{
    girara_info("🔊 DEBUG: play_text called with text: '%.50s%s'", 
                text ? text : "(null)", text && strlen(text) > 50 ? "..." : "");
    
    /* This function is deprecated in streaming-only mode */
    /* Text playback is now handled by the streaming engine */
    (void)controller;
    (void)text;
    girara_info("🔧 DEBUG: play_text called but deprecated in streaming-only mode");
    return true;
}

/* Old chunking system removed - now using streaming-only architecture */

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
    
    /* Note: Navigation during streaming playback would require streaming engine support */
    /* For now, just update position - streaming will continue from current point */
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
    
    /* Note: Navigation during streaming playback would require streaming engine support */
    /* For now, just update position - streaming will continue from current point */
    return true;
}

/* Engine integration */

void 
tts_audio_controller_set_engine(tts_audio_controller_t* controller, void* engine) 
{
    /* Engine setting deprecated in streaming-only mode */
    (void)controller;
    (void)engine;
    girara_info("🔧 DEBUG: set_engine called but deprecated in streaming-only mode");
}

void* 
tts_audio_controller_get_engine(tts_audio_controller_t* controller) 
{
    /* Engine getting deprecated in streaming-only mode */
    (void)controller;
    girara_info("🔧 DEBUG: get_engine called but deprecated in streaming-only mode");
    return NULL;
}
/* Streaming is now the only mode - no enable/disable needed */

/* Streaming-based session management */

static bool 
tts_audio_controller_start_streaming_session(tts_audio_controller_t* controller, girara_list_t* segments) 
{
    girara_info("🔧 DEBUG: start_streaming_session called with controller=%p, segments=%p", (void*)controller, (void*)segments);
    
    if (controller == NULL || segments == NULL) {
        girara_error("🚨 DEBUG: start_streaming_session - invalid parameters: controller=%p, segments=%p", (void*)controller, (void*)segments);
        return false;
    }
    
    girara_info("🔧 DEBUG: start_streaming_session - parameters valid, segments count: %zu", girara_list_size(segments));
    
    /* Create streaming engine if not exists */
    if (controller->streaming_engine == NULL) {
        tts_engine_type_t engine_type = TTS_ENGINE_PIPER;
        girara_info("🔧 DEBUG: Creating streaming engine with Piper-TTS");
        
        controller->streaming_engine = tts_streaming_engine_new(engine_type);
        if (controller->streaming_engine == NULL) {
            girara_error("Failed to create streaming TTS engine");
            return false;
        }
        
        girara_info("✅ DEBUG: Streaming TTS engine created (type: %d)", engine_type);
    }
    
    tts_streaming_engine_t* streaming_engine = (tts_streaming_engine_t*)controller->streaming_engine;
    
    girara_info("🚀 DEBUG: Starting streaming TTS session with %zu segments", girara_list_size(segments));
    
    /* Start the streaming engine */
    if (!tts_streaming_engine_start(streaming_engine)) {
        girara_error("Failed to start streaming TTS engine");
        return false;
    }
    
    /* Queue all text segments */
    for (size_t i = 0; i < girara_list_size(segments); i++) {
        tts_text_segment_t* segment = girara_list_nth(segments, i);
        if (segment != NULL && segment->text != NULL) {
            if (!tts_streaming_engine_queue_segment(streaming_engine, segment)) {
                girara_warning("Failed to queue segment %zu", i);
            } else {
                girara_debug("🔧 DEBUG: Queued segment %zu: '%.50s%s'", 
                           i, segment->text, strlen(segment->text) > 50 ? "..." : "");
            }
        }
    }
    
    girara_info("✅ DEBUG: Streaming session started with %zu segments queued", girara_list_size(segments));
    return true;
}

static void 
tts_audio_controller_stop_streaming_session(tts_audio_controller_t* controller) 
{
    if (controller == NULL || controller->streaming_engine == NULL) {
        return;
    }
    
    tts_streaming_engine_t* streaming_engine = (tts_streaming_engine_t*)controller->streaming_engine;
    
    girara_info("🔧 DEBUG: Stopping streaming TTS session");
    tts_streaming_engine_stop(streaming_engine);
    girara_info("✅ DEBUG: Streaming session stopped");
}