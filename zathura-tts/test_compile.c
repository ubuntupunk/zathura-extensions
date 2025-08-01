/* Simple compilation test for plugin.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <glib.h>

/* Mock zathura types and functions for compilation test */
typedef struct zathura_s zathura_t;
typedef enum {
    ZATHURA_ERROR_OK = 0,
    ZATHURA_ERROR_INVALID_ARGUMENTS,
    ZATHURA_ERROR_OUT_OF_MEMORY
} zathura_error_t;

/* Mock girara functions */
void girara_info(const char* format, ...) {}
void girara_error(const char* format, ...) {}
void girara_warning(const char* format, ...) {}

/* Mock config defines */
#define PLUGIN_NAME "zathura-tts"
#define PLUGIN_VERSION "0.1.0"
#define PLUGIN_API_VERSION "4"

/* Mock plugin registration macro */
#define ZATHURA_PLUGIN_REGISTER_WITH_FUNCTIONS(name, major, minor, rev, functions, mimetypes) \
    void zathura_plugin_register_service(void) {}

#define ZATHURA_PLUGIN_FUNCTIONS(funcs) funcs
#define ZATHURA_PLUGIN_MIMETYPES(types) types

/* Include the actual plugin header and source */
#include "src/plugin.h"

/* Mock the missing session types */
typedef struct tts_session_s {
    int dummy;
} tts_session_t;

typedef struct tts_config_s {
    int dummy;
} tts_config_t;

typedef struct tts_engine_s {
    int dummy;
} tts_engine_t;

typedef struct tts_audio_state_s {
    int dummy;
} tts_audio_state_t;

/* Now include the plugin source */
#include "src/plugin.c"

int main() {
    printf("Plugin compilation test successful!\n");
    return 0;
}