/* TTS Engine Implementation Headers
 * Common definitions for engine-specific implementations
 */

#ifndef TTS_ENGINE_IMPL_H
#define TTS_ENGINE_IMPL_H

#include "tts-engine.h"
#include <girara/log.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>

/* Helper function to check if a command exists (implemented in tts-engine.c) */
extern bool command_exists(const char* command);

/* Piper engine functions */
extern const tts_engine_functions_t piper_functions;

/* Speech Dispatcher engine functions */
extern const tts_engine_functions_t speech_dispatcher_functions;

/* espeak-ng engine functions */
extern const tts_engine_functions_t espeak_functions;

#endif /* TTS_ENGINE_IMPL_H */