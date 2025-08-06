/* TTS Engine Implementation Headers
 * Function tables for different TTS engines
 */

#ifndef TTS_ENGINE_IMPL_H
#define TTS_ENGINE_IMPL_H

#include "tts-engine.h"

/* Engine function tables - defined in separate implementation files */
extern const tts_engine_functions_t piper_functions;
extern const tts_engine_functions_t speech_dispatcher_functions;
extern const tts_engine_functions_t espeak_functions;

#endif /* TTS_ENGINE_IMPL_H */