#ifndef TTS_AUDIO_CONTROLLER_H
#define TTS_AUDIO_CONTROLLER_H

#include <glib.h>
#include <stdbool.h>
#include <girara/types.h>

/* Forward declarations */
typedef struct tts_audio_controller_s tts_audio_controller_t;
typedef struct tts_text_segment_s tts_text_segment_t;

/* Audio playback states */
typedef enum {
    TTS_AUDIO_STATE_STOPPED,
    TTS_AUDIO_STATE_PLAYING,
    TTS_AUDIO_STATE_PAUSED,
    TTS_AUDIO_STATE_ERROR
} tts_audio_state_t;

/* Text segment structure */
struct tts_text_segment_s {
    char* text;
    int page_number;
    int segment_id;
    size_t start_offset;
    size_t end_offset;
};

/* Audio controller state structure */
struct tts_audio_controller_s {
    /* State management */
    tts_audio_state_t state;
    GMutex state_mutex;
    GCond state_cond;
    
    /* Current playback position */
    int current_page;
    int current_segment;
    girara_list_t* text_segments;
    
    /* Audio settings */
    float speed_multiplier;
    int volume_level;
    
    /* Threading */
    GThread* audio_thread;
    bool should_stop;
    
    /* Current text being processed */
    char* current_text;
    
    /* TTS Engine integration */
    void* tts_engine;
    
    /* Callbacks for state changes */
    void (*state_change_callback)(tts_audio_state_t old_state, tts_audio_state_t new_state, void* user_data);
    void* callback_user_data;
};

/* Audio controller management functions */
tts_audio_controller_t* tts_audio_controller_new(void);
void tts_audio_controller_free(tts_audio_controller_t* controller);

/* State management functions */
tts_audio_state_t tts_audio_controller_get_state(tts_audio_controller_t* controller);
bool tts_audio_controller_set_state(tts_audio_controller_t* controller, tts_audio_state_t new_state);

/* Session management functions */
bool tts_audio_controller_start_session(tts_audio_controller_t* controller, girara_list_t* segments);
void tts_audio_controller_stop_session(tts_audio_controller_t* controller);
bool tts_audio_controller_pause_session(tts_audio_controller_t* controller);
bool tts_audio_controller_resume_session(tts_audio_controller_t* controller);

/* Position management functions */
int tts_audio_controller_get_current_page(tts_audio_controller_t* controller);
int tts_audio_controller_get_current_segment(tts_audio_controller_t* controller);
bool tts_audio_controller_set_position(tts_audio_controller_t* controller, int page, int segment);

/* Audio settings functions */
float tts_audio_controller_get_speed(tts_audio_controller_t* controller);
bool tts_audio_controller_set_speed(tts_audio_controller_t* controller, float speed);
int tts_audio_controller_get_volume(tts_audio_controller_t* controller);
bool tts_audio_controller_set_volume(tts_audio_controller_t* controller, int volume);

/* Thread synchronization functions */
void tts_audio_controller_lock(tts_audio_controller_t* controller);
void tts_audio_controller_unlock(tts_audio_controller_t* controller);
void tts_audio_controller_wait_for_state_change(tts_audio_controller_t* controller);
void tts_audio_controller_signal_state_change(tts_audio_controller_t* controller);

/* Callback management */
void tts_audio_controller_set_state_change_callback(tts_audio_controller_t* controller,
                                                   void (*callback)(tts_audio_state_t, tts_audio_state_t, void*),
                                                   void* user_data);

/* Playback control functions */
bool tts_audio_controller_play_text(tts_audio_controller_t* controller, const char* text);
bool tts_audio_controller_play_current_segment(tts_audio_controller_t* controller);
bool tts_audio_controller_navigate_to_segment(tts_audio_controller_t* controller, int direction);
bool tts_audio_controller_navigate_to_page(tts_audio_controller_t* controller, int page);

/* Engine integration */
void tts_audio_controller_set_engine(tts_audio_controller_t* controller, void* engine);
void* tts_audio_controller_get_engine(tts_audio_controller_t* controller);

/* Text segment helper functions */
tts_text_segment_t* tts_text_segment_new(const char* text, int page, int segment_id);
void tts_text_segment_free(tts_text_segment_t* segment);

#endif /* TTS_AUDIO_CONTROLLER_H */