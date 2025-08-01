#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>
#include <glib.h>

/* Mock zathura structures for testing */
typedef struct {
    char* name;
} mock_zathura_t;

typedef enum {
    ZATHURA_ERROR_OK = 0,
    ZATHURA_ERROR_INVALID_ARGUMENTS,
    ZATHURA_ERROR_OUT_OF_MEMORY
} zathura_error_t;

/* Mock girara logging functions */
void girara_info(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[INFO] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void girara_error(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[ERROR] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

void girara_warning(const char* format, ...) {
    va_list args;
    va_start(args, format);
    printf("[WARNING] ");
    vprintf(format, args);
    printf("\n");
    va_end(args);
}

/* Include our plugin header with mocked dependencies */
#define TTS_PLUGIN_NAME "zathura-tts"
#define TTS_PLUGIN_VERSION "0.1.0"
#define TTS_PLUGIN_API_VERSION "4"

typedef struct tts_session_s tts_session_t;
typedef struct tts_config_s tts_config_t;
typedef struct tts_engine_s tts_engine_t;
typedef struct tts_audio_state_s tts_audio_state_t;

typedef struct tts_plugin_s {
  char* name;
  char* version;
  mock_zathura_t* zathura;
  tts_session_t* session;
  bool initialized;
} tts_plugin_t;

/* Plugin functions */
zathura_error_t tts_plugin_register(mock_zathura_t* zathura);
zathura_error_t tts_plugin_init(mock_zathura_t* zathura);
void tts_plugin_cleanup(void);
tts_plugin_t* tts_plugin_get_instance(void);

/* Global plugin instance */
static tts_plugin_t* g_tts_plugin = NULL;

/* Plugin registration function */
zathura_error_t
tts_plugin_register(mock_zathura_t* zathura)
{
  if (zathura == NULL) {
    girara_error("TTS plugin registration failed: invalid zathura instance");
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  /* Check if already registered */
  if (g_tts_plugin != NULL && g_tts_plugin->initialized) {
    girara_warning("TTS plugin already registered");
    return ZATHURA_ERROR_OK;
  }

  /* Allocate plugin instance */
  g_tts_plugin = g_malloc0(sizeof(tts_plugin_t));
  if (g_tts_plugin == NULL) {
    girara_error("TTS plugin registration failed: memory allocation error");
    return ZATHURA_ERROR_OUT_OF_MEMORY;
  }

  /* Set plugin metadata */
  g_tts_plugin->name = g_strdup(TTS_PLUGIN_NAME);
  g_tts_plugin->version = g_strdup(TTS_PLUGIN_VERSION);
  g_tts_plugin->zathura = zathura;
  g_tts_plugin->session = NULL;
  g_tts_plugin->initialized = false;

  girara_info("TTS plugin registered successfully: %s v%s", 
              g_tts_plugin->name, g_tts_plugin->version);

  return ZATHURA_ERROR_OK;
}

zathura_error_t
tts_plugin_init(mock_zathura_t* zathura)
{
  if (zathura == NULL) {
    girara_error("TTS plugin initialization failed: invalid zathura instance");
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  /* Ensure plugin is registered first */
  if (g_tts_plugin == NULL) {
    zathura_error_t result = tts_plugin_register(zathura);
    if (result != ZATHURA_ERROR_OK) {
      return result;
    }
  }

  /* Check if already initialized */
  if (g_tts_plugin->initialized) {
    girara_warning("TTS plugin already initialized");
    return ZATHURA_ERROR_OK;
  }

  girara_info("Initializing TTS plugin...");

  /* Validate zathura instance matches registered instance */
  if (g_tts_plugin->zathura != zathura) {
    girara_error("TTS plugin initialization failed: zathura instance mismatch");
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  /* Mark as initialized */
  g_tts_plugin->initialized = true;

  girara_info("TTS plugin initialized successfully");
  return ZATHURA_ERROR_OK;
}

void
tts_plugin_cleanup(void)
{
  girara_info("Cleaning up TTS plugin...");

  if (g_tts_plugin == NULL) {
    girara_warning("TTS plugin cleanup called but plugin not initialized");
    return;
  }

  /* Mark as not initialized */
  g_tts_plugin->initialized = false;

  /* Free plugin metadata */
  if (g_tts_plugin->name != NULL) {
    g_free(g_tts_plugin->name);
    g_tts_plugin->name = NULL;
  }

  if (g_tts_plugin->version != NULL) {
    g_free(g_tts_plugin->version);
    g_tts_plugin->version = NULL;
  }

  /* Reset references */
  g_tts_plugin->zathura = NULL;
  g_tts_plugin->session = NULL;

  /* Free plugin instance */
  g_free(g_tts_plugin);
  g_tts_plugin = NULL;

  girara_info("TTS plugin cleanup completed");
}

tts_plugin_t*
tts_plugin_get_instance(void)
{
  return g_tts_plugin;
}

/* Test functions */
int test_plugin_registration() {
    printf("\n=== Testing Plugin Registration ===\n");
    
    mock_zathura_t mock_zathura = { .name = "test_zathura" };
    
    /* Test registration */
    zathura_error_t result = tts_plugin_register(&mock_zathura);
    if (result != ZATHURA_ERROR_OK) {
        printf("FAIL: Plugin registration failed\n");
        return 1;
    }
    
    /* Verify plugin instance */
    tts_plugin_t* plugin = tts_plugin_get_instance();
    if (plugin == NULL) {
        printf("FAIL: Plugin instance is NULL\n");
        return 1;
    }
    
    if (strcmp(plugin->name, TTS_PLUGIN_NAME) != 0) {
        printf("FAIL: Plugin name mismatch\n");
        return 1;
    }
    
    if (strcmp(plugin->version, TTS_PLUGIN_VERSION) != 0) {
        printf("FAIL: Plugin version mismatch\n");
        return 1;
    }
    
    if (plugin->zathura != &mock_zathura) {
        printf("FAIL: Zathura instance mismatch\n");
        return 1;
    }
    
    if (plugin->initialized) {
        printf("FAIL: Plugin should not be initialized after registration\n");
        return 1;
    }
    
    printf("PASS: Plugin registration successful\n");
    return 0;
}

int test_plugin_initialization() {
    printf("\n=== Testing Plugin Initialization ===\n");
    
    mock_zathura_t mock_zathura = { .name = "test_zathura" };
    
    /* Test initialization */
    zathura_error_t result = tts_plugin_init(&mock_zathura);
    if (result != ZATHURA_ERROR_OK) {
        printf("FAIL: Plugin initialization failed\n");
        return 1;
    }
    
    /* Verify plugin is initialized */
    tts_plugin_t* plugin = tts_plugin_get_instance();
    if (!plugin->initialized) {
        printf("FAIL: Plugin should be initialized\n");
        return 1;
    }
    
    /* Test double initialization */
    result = tts_plugin_init(&mock_zathura);
    if (result != ZATHURA_ERROR_OK) {
        printf("FAIL: Double initialization should succeed with warning\n");
        return 1;
    }
    
    printf("PASS: Plugin initialization successful\n");
    return 0;
}

int test_plugin_cleanup() {
    printf("\n=== Testing Plugin Cleanup ===\n");
    
    /* Test cleanup */
    tts_plugin_cleanup();
    
    /* Verify plugin instance is cleaned up */
    tts_plugin_t* plugin = tts_plugin_get_instance();
    if (plugin != NULL) {
        printf("FAIL: Plugin instance should be NULL after cleanup\n");
        return 1;
    }
    
    /* Test double cleanup */
    tts_plugin_cleanup();
    
    printf("PASS: Plugin cleanup successful\n");
    return 0;
}

int test_error_conditions() {
    printf("\n=== Testing Error Conditions ===\n");
    
    /* Test registration with NULL zathura */
    zathura_error_t result = tts_plugin_register(NULL);
    if (result != ZATHURA_ERROR_INVALID_ARGUMENTS) {
        printf("FAIL: Should return INVALID_ARGUMENTS for NULL zathura\n");
        return 1;
    }
    
    /* Test initialization with NULL zathura */
    result = tts_plugin_init(NULL);
    if (result != ZATHURA_ERROR_INVALID_ARGUMENTS) {
        printf("FAIL: Should return INVALID_ARGUMENTS for NULL zathura\n");
        return 1;
    }
    
    printf("PASS: Error conditions handled correctly\n");
    return 0;
}

int main() {
    printf("Running TTS Plugin Tests\n");
    
    int failures = 0;
    
    failures += test_plugin_registration();
    failures += test_plugin_initialization();
    failures += test_plugin_cleanup();
    failures += test_error_conditions();
    
    printf("\n=== Test Summary ===\n");
    if (failures == 0) {
        printf("All tests passed!\n");
        return 0;
    } else {
        printf("%d test(s) failed!\n", failures);
        return 1;
    }
}