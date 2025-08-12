#include "plugin.h"
#include "tts-engine.h"
#include "tts-audio-controller.h"
#include "tts-ui-controller.h"
#include "tts-config.h"
#include "tts-error.h"
#include <zathura/plugin-api.h>
#include <girara/utils.h>
#include <girara/log.h>
#include <girara/session.h>
#include <string.h>
#include <stdlib.h>

/* Forward declaration for zathura function */
extern girara_session_t* zathura_get_session(zathura_t* zathura);

/* Global plugin instance */
static tts_plugin_t* g_tts_plugin = NULL;

/* Plugin initialization function for utility plugin API */
bool
tts_plugin_init(zathura_t* zathura)
{
  if (zathura == NULL) {
    girara_error("TTS plugin initialization failed: invalid zathura instance");
    return false;
  }

  /* Allocate plugin instance if not already done */
  if (g_tts_plugin == NULL) {
    g_tts_plugin = g_malloc0(sizeof(tts_plugin_t));
    if (g_tts_plugin == NULL) {
      girara_error("TTS plugin initialization failed: memory allocation error");
      return false;
    }

    /* Set plugin metadata */
    g_tts_plugin->name = g_strdup("zathura-tts");
    g_tts_plugin->version = g_strdup("1.0.0");
    g_tts_plugin->zathura = zathura;
    g_tts_plugin->session = NULL;
    g_tts_plugin->initialized = false;
  }

  if (g_tts_plugin->initialized) {
    girara_warning("TTS plugin already initialized");
    return true;
  }

  girara_info("Initializing TTS plugin...");

  /* Get girara session */
  girara_session_t* girara_session = zathura_get_session(zathura);
  if (girara_session == NULL) {
    girara_error("TTS plugin initialization failed: girara session not available");
    return false;
  }

  /* Create TTS session */
  tts_session_t* session = g_malloc0(sizeof(tts_session_t));
  if (session == NULL) {
    girara_error("TTS plugin initialization failed: session allocation error");
    return false;
  }

  /* Initialize session */
  session->zathura = zathura;
  session->girara_session = girara_session;
  session->active = false;

  /* Store session in plugin */
  g_tts_plugin->session = session;

  /* 1. Initialize configuration */
  girara_info("Initializing TTS configuration...");
  session->config = tts_config_new();
  if (session->config == NULL) {
    girara_error("TTS plugin initialization failed: configuration initialization error");
    tts_plugin_cleanup();
    return false;
  }

  /* Load default configuration */
  if (!tts_config_load_default(session->config)) {
    girara_warning("Failed to load default TTS configuration, using built-in defaults");
  }

  /* 2. Initialize TTS engine */
  girara_info("Initializing TTS engine...");
  zathura_error_t engine_error = ZATHURA_ERROR_OK;
  session->engine = tts_engine_new(TTS_ENGINE_PIPER, &engine_error);
  if (session->engine == NULL) {
    girara_error("TTS plugin initialization failed: TTS engine initialization error: %d", engine_error);
    tts_plugin_cleanup();
    return false;
  }

  /* Initialize engine with configuration */
  tts_engine_config_t* engine_config = tts_engine_config_new();
  if (engine_config == NULL) {
    girara_error("TTS plugin initialization failed: engine config allocation error");
    tts_plugin_cleanup();
    return false;
  }
  
  /* Set engine config from main config */
  engine_config->speed = tts_config_get_default_speed(session->config);
  engine_config->volume = tts_config_get_default_volume(session->config);
  engine_config->pitch = tts_config_get_default_pitch(session->config);
  engine_config->voice_name = g_strdup(tts_config_get_preferred_voice(session->config));
  
  if (!tts_engine_init(session->engine, engine_config, &engine_error)) {
    girara_error("TTS plugin initialization failed: TTS engine setup error: %d", engine_error);
    tts_engine_config_free(engine_config);
    tts_plugin_cleanup();
    return false;
  }
  
  tts_engine_config_free(engine_config);

  /* 3. Initialize audio controller */
  girara_info("Initializing TTS audio controller...");
  session->audio_controller = tts_audio_controller_new();
  if (session->audio_controller == NULL) {
    girara_error("TTS plugin initialization failed: audio controller initialization error");
    tts_plugin_cleanup();
    return false;
  }

  /* 4. Initialize UI controller */
  girara_info("Initializing TTS UI controller...");
  session->ui_controller = tts_ui_controller_new(zathura, session->audio_controller);
  if (session->ui_controller == NULL) {
    girara_error("TTS plugin initialization failed: UI controller initialization error");
    tts_plugin_cleanup();
    return false;
  }

  /* Register keyboard shortcuts and commands */
  girara_info("Registering TTS shortcuts...");
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
  return true;
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

tts_plugin_t*
tts_plugin_get_instance(void)
{
  return g_tts_plugin;
}

/* Register the TTS plugin as a utility plugin */
ZATHURA_UTILITY_PLUGIN_REGISTER(
  "zathura-tts",
  1, 0, 0,
  tts_plugin_init
)