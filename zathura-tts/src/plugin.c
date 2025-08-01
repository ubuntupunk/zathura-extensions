#include "plugin.h"
#include <zathura/plugin-api.h>
#include <girara/utils.h>
#include <string.h>
#include <stdlib.h>

/* Global plugin instance */
static tts_plugin_t* g_tts_plugin = NULL;

/* Plugin registration function */
zathura_error_t
tts_plugin_register(zathura_t* zathura)
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
tts_plugin_init(zathura_t* zathura)
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

  /* TODO: Initialize TTS subsystems in future tasks */
  /* This will include:
   * - TTS engine initialization
   * - Audio controller setup
   * - UI controller registration
   * - Configuration loading
   */

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

  /* TODO: Cleanup TTS resources in future tasks */
  /* This will include:
   * - TTS engine cleanup
   * - Audio controller shutdown
   * - UI controller unregistration
   * - Configuration saving
   * - Session cleanup
   */

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

/* Plugin entry point - required by Zathura plugin system */
/* TTS plugin doesn't handle document formats, so no functions or mimetypes */
ZATHURA_PLUGIN_REGISTER_WITH_FUNCTIONS(
  TTS_PLUGIN_NAME,
  0, 1, 0,
  ZATHURA_PLUGIN_FUNCTIONS({}),
  ZATHURA_PLUGIN_MIMETYPES({})
)