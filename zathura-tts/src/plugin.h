#ifndef ZATHURA_TTS_PLUGIN_H
#define ZATHURA_TTS_PLUGIN_H

#include <zathura/plugin-api.h>
#include <girara/types.h>
#include <glib.h>

#include "config.h"

/* Plugin metadata */
#define TTS_PLUGIN_NAME PLUGIN_NAME
#define TTS_PLUGIN_VERSION PLUGIN_VERSION
#define TTS_PLUGIN_API_VERSION PLUGIN_API_VERSION

/* Forward declarations */
typedef struct tts_session_s tts_session_t;
typedef struct tts_config_s tts_config_t;
typedef struct tts_engine_s tts_engine_t;
typedef struct tts_audio_controller_s tts_audio_controller_t;
typedef struct tts_ui_controller_s tts_ui_controller_t;

/* TTS session structure containing all components */
struct tts_session_s {
  tts_config_t* config;
  tts_engine_t* engine;
  tts_audio_controller_t* audio_controller;
  tts_ui_controller_t* ui_controller;
  zathura_t* zathura;
  girara_session_t* girara_session;
  bool active;
};

/* Plugin metadata structure */
typedef struct tts_plugin_s {
  char* name;
  char* version;
  zathura_t* zathura;
  tts_session_t* session;
  bool initialized;
} tts_plugin_t;

/* Plugin functions */
zathura_error_t tts_plugin_register(zathura_t* zathura);
zathura_error_t tts_plugin_init(zathura_t* zathura);
void tts_plugin_cleanup(void);

/* Plugin state management */
bool tts_plugin_is_initialized(void);
zathura_error_t tts_plugin_validate_state(void);

/* Plugin instance access */
tts_plugin_t* tts_plugin_get_instance(void);

#endif /* ZATHURA_TTS_PLUGIN_H */