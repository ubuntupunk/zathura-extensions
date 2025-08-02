/* TTS Error Handling Implementation
 * Comprehensive error handling system for TTS functionality
 */

#include "tts-error.h"
#include <girara/utils.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

/* Global error callback */
static tts_error_callback_t g_error_callback = NULL;
static void* g_error_callback_user_data = NULL;

/* Error code to string mapping */
const char* 
tts_error_get_string(tts_error_t error_code) 
{
    switch (error_code) {
        case TTS_ERROR_OK:
            return "No error";
        case TTS_ERROR_INVALID_ARGUMENTS:
            return "Invalid arguments";
        case TTS_ERROR_OUT_OF_MEMORY:
            return "Out of memory";
        case TTS_ERROR_FILE_NOT_FOUND:
            return "File not found";
        case TTS_ERROR_FILE_READ_FAILED:
            return "File read failed";
        case TTS_ERROR_FILE_WRITE_FAILED:
            return "File write failed";
        case TTS_ERROR_INVALID_CONFIG:
            return "Invalid configuration";
        case TTS_ERROR_NO_ENGINE:
            return "No TTS engine available";
        case TTS_ERROR_ENGINE_INIT_FAILED:
            return "TTS engine initialization failed";
        case TTS_ERROR_ENGINE_NOT_AVAILABLE:
            return "TTS engine not available";
        case TTS_ERROR_ENGINE_SPEAK_FAILED:
            return "TTS engine speak failed";
        case TTS_ERROR_ENGINE_PAUSE_FAILED:
            return "TTS engine pause failed";
        case TTS_ERROR_ENGINE_STOP_FAILED:
            return "TTS engine stop failed";
        case TTS_ERROR_ENGINE_CONFIG_FAILED:
            return "TTS engine configuration failed";
        case TTS_ERROR_NO_TEXT:
            return "No text available";
        case TTS_ERROR_TEXT_EXTRACTION_FAILED:
            return "Text extraction failed";
        case TTS_ERROR_INVALID_PAGE:
            return "Invalid page";
        case TTS_ERROR_INVALID_SEGMENT:
            return "Invalid text segment";
        case TTS_ERROR_AUDIO_FAILED:
            return "Audio operation failed";
        case TTS_ERROR_STATE_INVALID:
            return "Invalid state transition";
        case TTS_ERROR_SESSION_FAILED:
            return "TTS session failed";
        case TTS_ERROR_UI_FAILED:
            return "UI operation failed";
        case TTS_ERROR_SHORTCUT_FAILED:
            return "Shortcut registration failed";
        case TTS_ERROR_COMMAND_FAILED:
            return "Command execution failed";
        case TTS_ERROR_PLUGIN_INIT_FAILED:
            return "Plugin initialization failed";
        case TTS_ERROR_PLUGIN_CLEANUP_FAILED:
            return "Plugin cleanup failed";
        case TTS_ERROR_UNKNOWN:
        default:
            return "Unknown error";
    }
}

const char* 
tts_error_get_severity_string(tts_error_severity_t severity) 
{
    switch (severity) {
        case TTS_ERROR_SEVERITY_INFO:
            return "INFO";
        case TTS_ERROR_SEVERITY_WARNING:
            return "WARNING";
        case TTS_ERROR_SEVERITY_ERROR:
            return "ERROR";
        case TTS_ERROR_SEVERITY_CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

char* 
tts_error_get_user_message(tts_error_t error_code, const char* details) 
{
    const char* base_message = NULL;
    
    switch (error_code) {
        case TTS_ERROR_NO_ENGINE:
            base_message = "No TTS engine is available. Please install Piper-TTS, Speech Dispatcher, or espeak-ng.";
            break;
        case TTS_ERROR_ENGINE_NOT_AVAILABLE:
            base_message = "The requested TTS engine is not available. Trying fallback engines.";
            break;
        case TTS_ERROR_NO_TEXT:
            base_message = "No readable text found on this page.";
            break;
        case TTS_ERROR_TEXT_EXTRACTION_FAILED:
            base_message = "Failed to extract text from the document.";
            break;
        case TTS_ERROR_INVALID_PAGE:
            base_message = "Cannot access the current page.";
            break;
        case TTS_ERROR_ENGINE_SPEAK_FAILED:
            base_message = "TTS engine failed to speak. Trying alternative engine.";
            break;
        case TTS_ERROR_AUDIO_FAILED:
            base_message = "Audio playback failed. Check your audio system.";
            break;
        case TTS_ERROR_INVALID_CONFIG:
            base_message = "Invalid TTS configuration. Using default settings.";
            break;
        case TTS_ERROR_FILE_NOT_FOUND:
            base_message = "TTS configuration file not found. Using default settings.";
            break;
        case TTS_ERROR_OUT_OF_MEMORY:
            base_message = "Insufficient memory for TTS operation.";
            break;
        default:
            base_message = tts_error_get_string(error_code);
            break;
    }
    
    if (details != NULL && strlen(details) > 0) {
        return g_strdup_printf("%s (%s)", base_message, details);
    } else {
        return g_strdup(base_message);
    }
}

/* Error context management */

tts_error_context_t* 
tts_error_context_new(tts_error_t error_code, 
                      tts_error_severity_t severity,
                      const char* message,
                      const char* details,
                      const char* component,
                      const char* function,
                      int line) 
{
    tts_error_context_t* context = g_malloc0(sizeof(tts_error_context_t));
    if (context == NULL) {
        return NULL;
    }
    
    context->error_code = error_code;
    context->severity = severity;
    context->message = message ? g_strdup(message) : NULL;
    context->details = details ? g_strdup(details) : NULL;
    context->component = component ? g_strdup(component) : NULL;
    context->function = function ? g_strdup(function) : NULL;
    context->line = line;
    context->timestamp = g_date_time_new_now_local();
    
    return context;
}

void 
tts_error_context_free(tts_error_context_t* context) 
{
    if (context == NULL) {
        return;
    }
    
    g_free(context->message);
    g_free(context->details);
    g_free(context->component);
    g_free(context->function);
    
    if (context->timestamp != NULL) {
        g_date_time_unref(context->timestamp);
    }
    
    g_free(context);
}

tts_error_context_t* 
tts_error_context_copy(const tts_error_context_t* context) 
{
    if (context == NULL) {
        return NULL;
    }
    
    return tts_error_context_new(
        context->error_code,
        context->severity,
        context->message,
        context->details,
        context->component,
        context->function,
        context->line
    );
}

/* Error reporting functions */

void 
tts_error_report(tts_error_t error_code, 
                 tts_error_severity_t severity,
                 const char* component,
                 const char* function,
                 int line,
                 const char* format, ...) 
{
    if (format == NULL) {
        return;
    }
    
    /* Format the message */
    va_list args;
    va_start(args, format);
    char* message = g_strdup_vprintf(format, args);
    va_end(args);
    
    /* Create error context */
    tts_error_context_t* context = tts_error_context_new(
        error_code, severity, message, NULL, component, function, line
    );
    
    if (context != NULL) {
        /* Report the error */
        tts_error_report_context(context);
        tts_error_context_free(context);
    }
    
    g_free(message);
}

void 
tts_error_report_context(const tts_error_context_t* context) 
{
    if (context == NULL) {
        return;
    }
    
    /* Log the error */
    tts_error_log(context);
    
    /* Call error callback if set */
    if (g_error_callback != NULL) {
        g_error_callback(context, g_error_callback_user_data);
    }
}

/* Error callback management */

void 
tts_error_set_callback(tts_error_callback_t callback, void* user_data) 
{
    g_error_callback = callback;
    g_error_callback_user_data = user_data;
}

void 
tts_error_clear_callback(void) 
{
    g_error_callback = NULL;
    g_error_callback_user_data = NULL;
}

/* Graceful degradation helpers */

bool 
tts_error_should_retry(tts_error_t error_code) 
{
    switch (error_code) {
        case TTS_ERROR_ENGINE_SPEAK_FAILED:
        case TTS_ERROR_ENGINE_PAUSE_FAILED:
        case TTS_ERROR_ENGINE_STOP_FAILED:
        case TTS_ERROR_AUDIO_FAILED:
        case TTS_ERROR_TEXT_EXTRACTION_FAILED:
            return true;
        default:
            return false;
    }
}

bool 
tts_error_is_recoverable(tts_error_t error_code) 
{
    switch (error_code) {
        case TTS_ERROR_OUT_OF_MEMORY:
        case TTS_ERROR_PLUGIN_INIT_FAILED:
        case TTS_ERROR_PLUGIN_CLEANUP_FAILED:
            return false;
        case TTS_ERROR_INVALID_ARGUMENTS:
        case TTS_ERROR_INVALID_CONFIG:
        case TTS_ERROR_NO_ENGINE:
        case TTS_ERROR_ENGINE_NOT_AVAILABLE:
        case TTS_ERROR_NO_TEXT:
        case TTS_ERROR_INVALID_PAGE:
        case TTS_ERROR_INVALID_SEGMENT:
            return true;
        default:
            return tts_error_should_retry(error_code);
    }
}

tts_error_t 
tts_error_get_fallback_action(tts_error_t error_code) 
{
    switch (error_code) {
        case TTS_ERROR_ENGINE_NOT_AVAILABLE:
        case TTS_ERROR_ENGINE_INIT_FAILED:
        case TTS_ERROR_ENGINE_SPEAK_FAILED:
            return TTS_ERROR_NO_ENGINE; /* Try next engine */
        case TTS_ERROR_INVALID_CONFIG:
        case TTS_ERROR_FILE_NOT_FOUND:
        case TTS_ERROR_FILE_READ_FAILED:
            return TTS_ERROR_OK; /* Use defaults */
        case TTS_ERROR_NO_TEXT:
        case TTS_ERROR_TEXT_EXTRACTION_FAILED:
            return TTS_ERROR_INVALID_PAGE; /* Try different approach */
        default:
            return TTS_ERROR_UNKNOWN;
    }
}

/* Error logging */

void 
tts_error_log(const tts_error_context_t* context) 
{
    if (context == NULL) {
        return;
    }
    
    /* Format timestamp */
    char* timestamp_str = NULL;
    if (context->timestamp != NULL) {
        timestamp_str = g_date_time_format(context->timestamp, "%Y-%m-%d %H:%M:%S");
    }
    
    /* Log to stderr for now - in a full implementation, this could use Girara's logging */
    fprintf(stderr, "[%s] TTS %s: %s",
            timestamp_str ? timestamp_str : "unknown",
            tts_error_get_severity_string(context->severity),
            tts_error_get_string(context->error_code));
    
    if (context->message != NULL) {
        fprintf(stderr, " - %s", context->message);
    }
    
    if (context->details != NULL) {
        fprintf(stderr, " (%s)", context->details);
    }
    
    if (context->component != NULL && context->function != NULL) {
        fprintf(stderr, " [%s:%s:%d]", context->component, context->function, context->line);
    }
    
    fprintf(stderr, "\n");
    
    g_free(timestamp_str);
}

void 
tts_error_log_simple(tts_error_t error_code, const char* message) 
{
    tts_error_context_t* context = tts_error_context_new(
        error_code, TTS_ERROR_SEVERITY_ERROR, message, NULL, "TTS", "unknown", 0
    );
    
    if (context != NULL) {
        tts_error_log(context);
        tts_error_context_free(context);
    }
}