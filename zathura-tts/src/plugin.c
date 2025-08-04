#include "plugin.h"
#include "tts-engine.h"
#include "tts-audio-controller.h"
#include "tts-ui-controller.h"
#include "tts-config.h"
#include "tts-error.h"
#include "zathura-stubs.h"
#include "plugin-api.h"
#include <girara/utils.h>
#include <string.h>
#include <stdlib.h>

/* Global plugin instance */
static tts_plugin_t* g_tts_plugin = NULL;

/* Global initialization flag */
static bool g_tts_config_registered = false;

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
              fprintf(stderr, "=== TTS PLUGIN LOADED SUCCESSFULLY ===\n");
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
      girara_error("TTS plugin initialization failed: registration error");
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
    tts_plugin_cleanup();
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  /* Initialize TTS session */
  g_tts_plugin->session = g_malloc0(sizeof(tts_session_t));
  if (g_tts_plugin->session == NULL) {
    girara_error("TTS plugin initialization failed: session allocation error");
    tts_plugin_cleanup();
    return ZATHURA_ERROR_OUT_OF_MEMORY;
  }

  tts_session_t* session = g_tts_plugin->session;
  session->zathura = zathura;
  session->girara_session = NULL; /* Will be set by UI controller when needed */
  session->active = false;

  /* Initialize TTS subsystems */
  girara_info("Initializing TTS subsystems...");

  /* 1. Initialize configuration and register settings with Zathura */
  session->config = tts_config_new();
  if (session->config == NULL) {
    girara_error("TTS plugin initialization failed: config allocation error");
    tts_plugin_cleanup();
    return ZATHURA_ERROR_OUT_OF_MEMORY;
  }
  
  /* Register TTS configuration options with Zathura's configuration system */
  girara_session_t* girara_session = zathura_get_session(zathura);
  if (girara_session == NULL) {
    girara_error("TTS plugin initialization failed: could not get girara session");
    tts_plugin_cleanup();
    return ZATHURA_ERROR_UNKNOWN;
  }
  
  session->girara_session = girara_session;
  
  /* Register all TTS configuration options */
  if (!tts_config_register_settings(session->config, girara_session)) {
    girara_error("TTS plugin initialization failed: could not register configuration settings");
    tts_plugin_cleanup();
    return ZATHURA_ERROR_UNKNOWN;
  }
  
  /* Load configuration values from Zathura's settings */
  if (!tts_config_load_from_zathura(session->config, girara_session)) {
    girara_warning("Failed to load some TTS configuration values, using defaults");
  }

  /* 2. Initialize TTS engine */
  zathura_error_t engine_error;
  tts_engine_type_t preferred_engine = tts_config_get_preferred_engine(session->config);
  
  /* Try preferred engine first, then fallback to available engines */
  session->engine = tts_engine_new(preferred_engine, &engine_error);
  if (session->engine == NULL || !session->engine->is_available) {
    girara_warning("Preferred TTS engine not available, trying fallbacks...");
    
    /* Try to get any available engine */
    tts_engine_type_t fallback_engine = tts_engine_get_preferred_type(&engine_error);
    if (fallback_engine != TTS_ENGINE_NONE) {
      if (session->engine != NULL) {
        tts_engine_free(session->engine);
      }
      session->engine = tts_engine_new(fallback_engine, &engine_error);
    }
  }

  if (session->engine == NULL || !session->engine->is_available) {
    girara_error("TTS plugin initialization failed: no TTS engine available");
    tts_plugin_cleanup();
    return ZATHURA_ERROR_UNKNOWN;
  }

  /* Initialize the engine with configuration */
  tts_engine_config_t* engine_config = tts_engine_config_new();
  if (engine_config != NULL) {
    engine_config->speed = tts_config_get_default_speed(session->config);
    engine_config->volume = tts_config_get_default_volume(session->config);
    engine_config->voice_name = g_strdup(tts_config_get_preferred_voice(session->config));
    
    if (!tts_engine_init(session->engine, engine_config, &engine_error)) {
      girara_warning("TTS engine initialization failed, continuing with defaults");
    }
    
    tts_engine_config_free(engine_config);
  }

  /* 3. Initialize audio controller */
  session->audio_controller = tts_audio_controller_new();
  if (session->audio_controller == NULL) {
    girara_error("TTS plugin initialization failed: audio controller allocation error");
    tts_plugin_cleanup();
    return ZATHURA_ERROR_OUT_OF_MEMORY;
  }

  /* Connect audio controller to TTS engine */
  tts_audio_controller_set_engine(session->audio_controller, session->engine);

  /* Set audio settings from configuration */
  tts_audio_controller_set_speed(session->audio_controller, tts_config_get_default_speed(session->config));
  tts_audio_controller_set_volume(session->audio_controller, tts_config_get_default_volume(session->config));

  /* 4. Initialize UI controller */
  session->ui_controller = tts_ui_controller_new(zathura, session->audio_controller);
  if (session->ui_controller == NULL) {
    girara_error("TTS plugin initialization failed: UI controller allocation error");
    tts_plugin_cleanup();
    return ZATHURA_ERROR_OUT_OF_MEMORY;
  }

  /* Register keyboard shortcuts and commands */
  if (!tts_ui_controller_register_shortcuts(session->ui_controller)) {
    girara_warning("Some TTS keyboard shortcuts failed to register");
  }

  if (!tts_ui_controller_register_commands(session->ui_controller)) {
    girara_warning("Some TTS commands failed to register");
  }

  /* Initialize visual feedback and notifications */
  if (!tts_ui_controller_init_visual_feedback(session->ui_controller)) {
    girara_warning("TTS visual feedback initialization failed");
  }

  if (!tts_ui_controller_init_notifications(session->ui_controller)) {
    girara_warning("TTS notifications initialization failed");
  }

  /* Mark session as active */
  session->active = true;

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

  /* Cleanup TTS resources */
  if (g_tts_plugin->session != NULL) {
    tts_session_t* session = g_tts_plugin->session;
    
    girara_info("Cleaning up TTS session...");
    
    /* Mark session as inactive */
    session->active = false;
    
    /* 1. Cleanup UI controller */
    if (session->ui_controller != NULL) {
      girara_info("Cleaning up TTS UI controller...");
      
      /* Unregister shortcuts and commands */
      tts_ui_controller_unregister_shortcuts(session->ui_controller);
      tts_ui_controller_unregister_commands(session->ui_controller);
      
      /* Free UI controller */
      tts_ui_controller_free(session->ui_controller);
      session->ui_controller = NULL;
    }
    
    /* 2. Cleanup audio controller */
    if (session->audio_controller != NULL) {
      girara_info("Cleaning up TTS audio controller...");
      
      /* Stop any active TTS session */
      tts_audio_controller_stop_session(session->audio_controller);
      
      /* Free audio controller */
      tts_audio_controller_free(session->audio_controller);
      session->audio_controller = NULL;
    }
    
    /* 3. Cleanup TTS engine */
    if (session->engine != NULL) {
      girara_info("Cleaning up TTS engine...");
      
      /* Cleanup and free engine */
      tts_engine_cleanup(session->engine);
      tts_engine_free(session->engine);
      session->engine = NULL;
    }
    
    /* 4. Save and cleanup configuration */
    if (session->config != NULL) {
      girara_info("Saving and cleaning up TTS configuration...");
      
      /* Save configuration if modified */
      if (tts_config_is_modified(session->config)) {
        if (!tts_config_save_default(session->config)) {
          girara_warning("Failed to save TTS configuration");
        }
      }
      
      /* Free configuration */
      tts_config_free(session->config);
      session->config = NULL;
    }
    
    /* Clear session references */
    session->zathura = NULL;
    session->girara_session = NULL;
    
    /* Free session */
    g_free(g_tts_plugin->session);
    g_tts_plugin->session = NULL;
    
    girara_info("TTS session cleanup completed");
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

  /* Free plugin instance */
  g_free(g_tts_plugin);
  g_tts_plugin = NULL;

  girara_info("TTS plugin cleanup completed");
}

bool
tts_plugin_is_initialized(void)
{
  return (g_tts_plugin != NULL && g_tts_plugin->initialized);
}

zathura_error_t
tts_plugin_validate_state(void)
{
  if (g_tts_plugin == NULL) {
    girara_error("TTS plugin state validation failed: plugin not registered");
    return ZATHURA_ERROR_UNKNOWN;
  }

  if (!g_tts_plugin->initialized) {
    girara_error("TTS plugin state validation failed: plugin not initialized");
    return ZATHURA_ERROR_UNKNOWN;
  }

  if (g_tts_plugin->zathura == NULL) {
    girara_error("TTS plugin state validation failed: invalid zathura instance");
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  /* Validate session */
  if (g_tts_plugin->session == NULL) {
    girara_error("TTS plugin state validation failed: session not initialized");
    return ZATHURA_ERROR_UNKNOWN;
  }

  tts_session_t* session = g_tts_plugin->session;
  
  if (!session->active) {
    girara_error("TTS plugin state validation failed: session not active");
    return ZATHURA_ERROR_UNKNOWN;
  }

  if (session->zathura != g_tts_plugin->zathura) {
    girara_error("TTS plugin state validation failed: session zathura mismatch");
    return ZATHURA_ERROR_INVALID_ARGUMENTS;
  }

  /* Validate core components */
  if (session->config == NULL) {
    girara_error("TTS plugin state validation failed: configuration not loaded");
    return ZATHURA_ERROR_UNKNOWN;
  }

  if (session->engine == NULL) {
    girara_error("TTS plugin state validation failed: TTS engine not initialized");
    return ZATHURA_ERROR_UNKNOWN;
  }

  if (session->audio_controller == NULL) {
    girara_error("TTS plugin state validation failed: audio controller not initialized");
    return ZATHURA_ERROR_UNKNOWN;
  }

  if (session->ui_controller == NULL) {
    girara_error("TTS plugin state validation failed: UI controller not initialized");
    return ZATHURA_ERROR_UNKNOWN;
  }

  return ZATHURA_ERROR_OK;
}

tts_plugin_t*
tts_plugin_get_instance(void)
{
  return g_tts_plugin;
}

/* Utility plugin initialization function */
static bool
tts_utility_plugin_init(zathura_t* zathura)
{
  if (zathura == NULL) {
    girara_error("TTS utility plugin initialization failed: invalid zathura instance");
    return false;
  }

  girara_info("Initializing TTS utility plugin...");

  /* Get girara session for configuration registration */
  girara_session_t* girara_session = zathura_get_session(zathura);
  if (girara_session == NULL) {
    girara_warning("TTS utility plugin: girara session not ready yet, deferring configuration registration");
    /* Continue with plugin registration without configuration options for now */
  } else {
    /* Register TTS configuration options if session is available */
    if (!g_tts_config_registered) {
      tts_config_t* temp_config = tts_config_new();
      if (temp_config != NULL) {
        if (tts_config_register_settings(temp_config, girara_session)) {
          girara_info("TTS configuration options registered successfully");
          g_tts_config_registered = true;
        } else {
          girara_warning("Failed to register TTS configuration options, will retry later");
        }
        tts_config_free(temp_config);
      } else {
        girara_warning("Failed to create temporary config for registration");
      }
    }
  }

  /* Register and initialize the TTS plugin */
  zathura_error_t result = tts_plugin_register(zathura);
  if (result != ZATHURA_ERROR_OK) {
    girara_error("Failed to register TTS plugin: %d", result);
    return false;
  }

  result = tts_plugin_init(zathura);
  if (result != ZATHURA_ERROR_OK) {
    girara_error("Failed to initialize TTS plugin: %d", result);
    tts_plugin_cleanup();
    return false;
  }

  girara_info("TTS utility plugin initialized successfully");
  return true;
}

/* Register the TTS plugin as a utility plugin */
ZATHURA_UTILITY_PLUGIN_REGISTER(
  "zathura-tts",
  0, 1, 0,
  tts_utility_plugin_init
)