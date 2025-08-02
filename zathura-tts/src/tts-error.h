#ifndef TTS_ERROR_H
#define TTS_ERROR_H

#include <glib.h>
#include <stdbool.h>

/* TTS Error codes */
typedef enum {
    TTS_ERROR_OK = 0,                    /**< No error */
    TTS_ERROR_INVALID_ARGUMENTS,         /**< Invalid function arguments */
    TTS_ERROR_OUT_OF_MEMORY,             /**< Memory allocation failed */
    TTS_ERROR_FILE_NOT_FOUND,            /**< Configuration file not found */
    TTS_ERROR_FILE_READ_FAILED,          /**< Failed to read file */
    TTS_ERROR_FILE_WRITE_FAILED,         /**< Failed to write file */
    TTS_ERROR_INVALID_CONFIG,            /**< Invalid configuration */
    TTS_ERROR_NO_ENGINE,                 /**< No TTS engine available */
    TTS_ERROR_ENGINE_INIT_FAILED,        /**< TTS engine initialization failed */
    TTS_ERROR_ENGINE_NOT_AVAILABLE,      /**< Requested TTS engine not available */
    TTS_ERROR_ENGINE_SPEAK_FAILED,       /**< TTS engine failed to speak */
    TTS_ERROR_ENGINE_PAUSE_FAILED,       /**< TTS engine failed to pause */
    TTS_ERROR_ENGINE_STOP_FAILED,        /**< TTS engine failed to stop */
    TTS_ERROR_ENGINE_CONFIG_FAILED,      /**< TTS engine configuration failed */
    TTS_ERROR_NO_TEXT,                   /**< No text available for TTS */
    TTS_ERROR_TEXT_EXTRACTION_FAILED,    /**< Text extraction failed */
    TTS_ERROR_INVALID_PAGE,              /**< Invalid page number */
    TTS_ERROR_INVALID_SEGMENT,           /**< Invalid text segment */
    TTS_ERROR_AUDIO_FAILED,              /**< Audio operation failed */
    TTS_ERROR_STATE_INVALID,             /**< Invalid state transition */
    TTS_ERROR_SESSION_FAILED,            /**< TTS session operation failed */
    TTS_ERROR_UI_FAILED,                 /**< UI operation failed */
    TTS_ERROR_SHORTCUT_FAILED,           /**< Shortcut registration failed */
    TTS_ERROR_COMMAND_FAILED,            /**< Command execution failed */
    TTS_ERROR_PLUGIN_INIT_FAILED,        /**< Plugin initialization failed */
    TTS_ERROR_PLUGIN_CLEANUP_FAILED,     /**< Plugin cleanup failed */
    TTS_ERROR_UNKNOWN                    /**< Unknown error */
} tts_error_t;

/* Error severity levels */
typedef enum {
    TTS_ERROR_SEVERITY_INFO,             /**< Informational message */
    TTS_ERROR_SEVERITY_WARNING,          /**< Warning message */
    TTS_ERROR_SEVERITY_ERROR,            /**< Error message */
    TTS_ERROR_SEVERITY_CRITICAL          /**< Critical error message */
} tts_error_severity_t;

/* Error context information */
typedef struct {
    tts_error_t error_code;              /**< Error code */
    tts_error_severity_t severity;       /**< Error severity */
    char* message;                       /**< Human-readable error message */
    char* details;                       /**< Additional error details */
    char* component;                     /**< Component that generated the error */
    char* function;                      /**< Function where error occurred */
    int line;                            /**< Line number where error occurred */
    GDateTime* timestamp;                /**< When the error occurred */
} tts_error_context_t;

/* Error callback function type */
typedef void (*tts_error_callback_t)(const tts_error_context_t* error_context, void* user_data);

/* Error handling functions */
const char* tts_error_get_string(tts_error_t error_code);
const char* tts_error_get_severity_string(tts_error_severity_t severity);
char* tts_error_get_user_message(tts_error_t error_code, const char* details);

/* Error context management */
tts_error_context_t* tts_error_context_new(tts_error_t error_code, 
                                           tts_error_severity_t severity,
                                           const char* message,
                                           const char* details,
                                           const char* component,
                                           const char* function,
                                           int line);
void tts_error_context_free(tts_error_context_t* context);
tts_error_context_t* tts_error_context_copy(const tts_error_context_t* context);

/* Error reporting functions */
void tts_error_report(tts_error_t error_code, 
                      tts_error_severity_t severity,
                      const char* component,
                      const char* function,
                      int line,
                      const char* format, ...);

void tts_error_report_context(const tts_error_context_t* context);

/* Error callback management */
void tts_error_set_callback(tts_error_callback_t callback, void* user_data);
void tts_error_clear_callback(void);

/* Graceful degradation helpers */
bool tts_error_should_retry(tts_error_t error_code);
bool tts_error_is_recoverable(tts_error_t error_code);
tts_error_t tts_error_get_fallback_action(tts_error_t error_code);

/* Error logging */
void tts_error_log(const tts_error_context_t* context);
void tts_error_log_simple(tts_error_t error_code, const char* message);

/* Convenience macros for error reporting */
#define TTS_ERROR_REPORT(code, severity, format, ...) \
    tts_error_report(code, severity, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#define TTS_ERROR_REPORT_SIMPLE(code, message) \
    tts_error_report(code, TTS_ERROR_SEVERITY_ERROR, __FILE__, __func__, __LINE__, "%s", message)

#define TTS_ERROR_REPORT_WARNING(code, format, ...) \
    tts_error_report(code, TTS_ERROR_SEVERITY_WARNING, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#define TTS_ERROR_REPORT_CRITICAL(code, format, ...) \
    tts_error_report(code, TTS_ERROR_SEVERITY_CRITICAL, __FILE__, __func__, __LINE__, format, ##__VA_ARGS__)

#endif /* TTS_ERROR_H */