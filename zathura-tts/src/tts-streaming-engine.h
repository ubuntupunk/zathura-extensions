/* TTS Streaming Engine Header
 * Provides continuous TTS audio streaming without process spawning overhead
 */

#ifndef TTS_STREAMING_ENGINE_H
#define TTS_STREAMING_ENGINE_H

#include <glib.h>
#include <gio/gio.h>
#include "tts-engine.h"

/* Include text segment definition from text extractor */
#include "tts-text-extractor.h"

/* Forward declarations */
typedef struct tts_streaming_engine_s tts_streaming_engine_t;

/* Streaming engine state */
typedef enum {
    TTS_STREAMING_STATE_IDLE,
    TTS_STREAMING_STATE_STARTING,
    TTS_STREAMING_STATE_ACTIVE,
    TTS_STREAMING_STATE_PAUSED,
    TTS_STREAMING_STATE_STOPPING,
    TTS_STREAMING_STATE_ERROR
} tts_streaming_state_t;

/* Streaming engine structure */
struct tts_streaming_engine_s {
    /* Process management */
    GPid process_pid;
    int stdin_fd;
    int stdout_fd;
    GIOChannel* text_channel;
    GIOChannel* audio_channel;
    
    /* State management */
    tts_streaming_state_t state;
    GMutex state_mutex;
    GCond state_cond;
    
    /* Text queue */
    GQueue* text_queue;
    GMutex queue_mutex;
    GCond queue_cond;
    GThread* feeder_thread;
    bool should_stop_feeding;
    bool is_paused;
    
    /* Audio management */
    GThread* audio_thread;
    bool should_stop_audio;
    
    /* Engine configuration */
    tts_engine_type_t engine_type;
    float speed;
    int volume;
    char* voice_name;
    
    /* Callbacks */
    void (*segment_finished_callback)(int segment_id, void* user_data);
    void (*state_changed_callback)(tts_streaming_state_t old_state, tts_streaming_state_t new_state, void* user_data);
    void* callback_user_data;
};

/* Streaming engine management */
tts_streaming_engine_t* tts_streaming_engine_new(tts_engine_type_t engine_type);
void tts_streaming_engine_free(tts_streaming_engine_t* engine);

/* Engine control */
bool tts_streaming_engine_start(tts_streaming_engine_t* engine);
bool tts_streaming_engine_stop(tts_streaming_engine_t* engine);
bool tts_streaming_engine_pause(tts_streaming_engine_t* engine);
bool tts_streaming_engine_resume(tts_streaming_engine_t* engine);
bool tts_streaming_engine_pause(tts_streaming_engine_t* engine);
bool tts_streaming_engine_resume(tts_streaming_engine_t* engine);

/* Text management */
bool tts_streaming_engine_queue_segment(tts_streaming_engine_t* engine, tts_text_segment_t* segment);
bool tts_streaming_engine_queue_text(tts_streaming_engine_t* engine, const char* text, int segment_id);
bool tts_streaming_engine_clear_queue(tts_streaming_engine_t* engine);
size_t tts_streaming_engine_get_queue_size(tts_streaming_engine_t* engine);

/* Configuration */
bool tts_streaming_engine_set_speed(tts_streaming_engine_t* engine, float speed);
bool tts_streaming_engine_set_volume(tts_streaming_engine_t* engine, int volume);
bool tts_streaming_engine_set_voice(tts_streaming_engine_t* engine, const char* voice_name);

/* State queries */
tts_streaming_state_t tts_streaming_engine_get_state(tts_streaming_engine_t* engine);
bool tts_streaming_engine_is_active(tts_streaming_engine_t* engine);

/* Callbacks */
void tts_streaming_engine_set_segment_finished_callback(tts_streaming_engine_t* engine,
                                                       void (*callback)(int segment_id, void* user_data),
                                                       void* user_data);
void tts_streaming_engine_set_state_changed_callback(tts_streaming_engine_t* engine,
                                                    void (*callback)(tts_streaming_state_t old_state, tts_streaming_state_t new_state, void* user_data),
                                                    void* user_data);

/* Note: Text segment utility functions are provided by tts-text-extractor.h */

#endif /* TTS_STREAMING_ENGINE_H */