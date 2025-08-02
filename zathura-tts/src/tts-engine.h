#ifndef TTS_ENGINE_H
#define TTS_ENGINE_H

#include <glib.h>
#include <stdbool.h>

/* Include Zathura types */
#include <zathura/types.h>

/**
 * TTS Engine types
 */
typedef enum {
    TTS_ENGINE_PIPER,           /**< Piper-TTS (neural voices) */
    TTS_ENGINE_SPEECH_DISPATCHER, /**< Speech Dispatcher */
    TTS_ENGINE_ESPEAK,          /**< espeak-ng */
    TTS_ENGINE_SYSTEM,          /**< System-specific TTS */
    TTS_ENGINE_NONE             /**< No engine available */
} tts_engine_type_t;

/**
 * TTS Engine state
 */
typedef enum {
    TTS_ENGINE_STATE_IDLE,      /**< Engine is idle */
    TTS_ENGINE_STATE_SPEAKING,  /**< Engine is currently speaking */
    TTS_ENGINE_STATE_PAUSED,    /**< Engine is paused */
    TTS_ENGINE_STATE_ERROR      /**< Engine is in error state */
} tts_engine_state_t;

/**
 * Voice information structure
 */
typedef struct {
    char* name;                 /**< Voice name */
    char* language;             /**< Language code (e.g., "en-US") */
    char* gender;               /**< Voice gender ("male", "female", "neutral") */
    int quality;                /**< Voice quality rating (0-100) */
} tts_voice_info_t;

/**
 * TTS Engine configuration
 */
typedef struct {
    float speed;                /**< Speech speed (0.5 - 3.0) */
    int volume;                 /**< Volume level (0-100) */
    char* voice_name;           /**< Selected voice name */
    int pitch;                  /**< Voice pitch (-50 to +50) */
} tts_engine_config_t;

/**
 * Forward declaration of TTS engine structure
 */
typedef struct tts_engine_s tts_engine_t;

/**
 * TTS Engine function pointers
 */
typedef struct {
    /**
     * Initialize the TTS engine
     *
     * @param engine The engine instance
     * @param config Initial configuration
     * @param error Set to an error value if an error occurred
     * @return true on success, false on error
     */
    bool (*init)(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
    
    /**
     * Cleanup and shutdown the TTS engine
     *
     * @param engine The engine instance
     */
    void (*cleanup)(tts_engine_t* engine);
    
    /**
     * Speak the given text
     *
     * @param engine The engine instance
     * @param text The text to speak
     * @param error Set to an error value if an error occurred
     * @return true on success, false on error
     */
    bool (*speak)(tts_engine_t* engine, const char* text, zathura_error_t* error);
    
    /**
     * Pause or resume speech
     *
     * @param engine The engine instance
     * @param pause true to pause, false to resume
     * @param error Set to an error value if an error occurred
     * @return true on success, false on error
     */
    bool (*pause)(tts_engine_t* engine, bool pause, zathura_error_t* error);
    
    /**
     * Stop current speech
     *
     * @param engine The engine instance
     * @param error Set to an error value if an error occurred
     * @return true on success, false on error
     */
    bool (*stop)(tts_engine_t* engine, zathura_error_t* error);
    
    /**
     * Set voice configuration
     *
     * @param engine The engine instance
     * @param config New voice configuration
     * @param error Set to an error value if an error occurred
     * @return true on success, false on error
     */
    bool (*set_config)(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);
    
    /**
     * Get current engine state
     *
     * @param engine The engine instance
     * @return Current engine state
     */
    tts_engine_state_t (*get_state)(tts_engine_t* engine);
    
    /**
     * Get list of available voices
     *
     * @param engine The engine instance
     * @param error Set to an error value if an error occurred
     * @return List of tts_voice_info_t* or NULL on error
     */
    girara_list_t* (*get_voices)(tts_engine_t* engine, zathura_error_t* error);
    
} tts_engine_functions_t;

/**
 * TTS Engine structure
 */
struct tts_engine_s {
    tts_engine_type_t type;             /**< Engine type */
    tts_engine_functions_t functions;   /**< Engine function pointers */
    tts_engine_config_t config;         /**< Current configuration */
    tts_engine_state_t state;           /**< Current state */
    void* engine_data;                  /**< Engine-specific data */
    char* name;                         /**< Engine name */
    bool is_available;                  /**< Whether engine is available */
};

/**
 * Create a new TTS engine instance
 *
 * @param type The type of engine to create
 * @param error Set to an error value if an error occurred
 * @return New engine instance or NULL on error
 */
tts_engine_t* tts_engine_new(tts_engine_type_t type, zathura_error_t* error);

/**
 * Free a TTS engine instance
 *
 * @param engine The engine to free
 */
void tts_engine_free(tts_engine_t* engine);

/**
 * Initialize a TTS engine with configuration
 *
 * @param engine The engine to initialize
 * @param config Initial configuration
 * @param error Set to an error value if an error occurred
 * @return true on success, false on error
 */
bool tts_engine_init(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);

/**
 * Cleanup and shutdown a TTS engine
 *
 * @param engine The engine to cleanup
 */
void tts_engine_cleanup(tts_engine_t* engine);

/**
 * Speak text using the TTS engine
 *
 * @param engine The engine to use
 * @param text The text to speak
 * @param error Set to an error value if an error occurred
 * @return true on success, false on error
 */
bool tts_engine_speak(tts_engine_t* engine, const char* text, zathura_error_t* error);

/**
 * Pause or resume speech
 *
 * @param engine The engine to control
 * @param pause true to pause, false to resume
 * @param error Set to an error value if an error occurred
 * @return true on success, false on error
 */
bool tts_engine_pause(tts_engine_t* engine, bool pause, zathura_error_t* error);

/**
 * Stop current speech
 *
 * @param engine The engine to control
 * @param error Set to an error value if an error occurred
 * @return true on success, false on error
 */
bool tts_engine_stop(tts_engine_t* engine, zathura_error_t* error);

/**
 * Set voice configuration
 *
 * @param engine The engine to configure
 * @param config New voice configuration
 * @param error Set to an error value if an error occurred
 * @return true on success, false on error
 */
bool tts_engine_set_config(tts_engine_t* engine, tts_engine_config_t* config, zathura_error_t* error);

/**
 * Get current engine state
 *
 * @param engine The engine to query
 * @return Current engine state
 */
tts_engine_state_t tts_engine_get_state(tts_engine_t* engine);

/**
 * Get list of available voices
 *
 * @param engine The engine to query
 * @param error Set to an error value if an error occurred
 * @return List of tts_voice_info_t* or NULL on error
 */
girara_list_t* tts_engine_get_voices(tts_engine_t* engine, zathura_error_t* error);

/**
 * Detect available TTS engines on the system
 *
 * @param error Set to an error value if an error occurred
 * @return List of available tts_engine_type_t values or NULL on error
 */
girara_list_t* tts_engine_detect_available(zathura_error_t* error);

/**
 * Get the preferred engine type based on availability and quality
 *
 * @param error Set to an error value if an error occurred
 * @return Preferred engine type or TTS_ENGINE_NONE if none available
 */
tts_engine_type_t tts_engine_get_preferred_type(zathura_error_t* error);

/**
 * Get engine type name as string
 *
 * @param type The engine type
 * @return Engine type name or "Unknown"
 */
const char* tts_engine_type_to_string(tts_engine_type_t type);

/**
 * Create a new voice info structure
 *
 * @param name Voice name
 * @param language Language code
 * @param gender Voice gender
 * @param quality Voice quality (0-100)
 * @return New voice info or NULL on error
 */
tts_voice_info_t* tts_voice_info_new(const char* name, const char* language, 
                                     const char* gender, int quality);

/**
 * Free a voice info structure
 *
 * @param voice_info The voice info to free
 */
void tts_voice_info_free(tts_voice_info_t* voice_info);

/**
 * Create a new engine configuration with defaults
 *
 * @return New configuration structure
 */
tts_engine_config_t* tts_engine_config_new(void);

/**
 * Free an engine configuration
 *
 * @param config The configuration to free
 */
void tts_engine_config_free(tts_engine_config_t* config);

/**
 * Copy an engine configuration
 *
 * @param config The configuration to copy
 * @return New configuration copy or NULL on error
 */
tts_engine_config_t* tts_engine_config_copy(const tts_engine_config_t* config);

#endif /* TTS_ENGINE_H */