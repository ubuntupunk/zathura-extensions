/* TTS Streaming Engine Implementation
 * Provides continuous TTS audio streaming without process spawning overhead
 */

#define _DEFAULT_SOURCE
#include "tts-streaming-engine.h"
#include "tts-text-extractor.h"
#include <girara/log.h>
#include <girara/utils.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

/* Internal function declarations */
static gpointer tts_text_feeder_thread(gpointer data);
static gpointer tts_audio_player_thread(gpointer data);
static bool tts_streaming_engine_spawn_process(tts_streaming_engine_t* engine);
static void tts_streaming_engine_cleanup_process(tts_streaming_engine_t* engine);
static bool tts_streaming_engine_set_state(tts_streaming_engine_t* engine, tts_streaming_state_t new_state);

/* Streaming engine management */

tts_streaming_engine_t* 
tts_streaming_engine_new(tts_engine_type_t engine_type) 
{
    tts_streaming_engine_t* engine = g_malloc0(sizeof(tts_streaming_engine_t));
    if (engine == NULL) {
        return NULL;
    }
    
    /* Initialize process management */
    engine->process_pid = 0;
    engine->stdin_fd = -1;
    engine->stdout_fd = -1;
    engine->text_channel = NULL;
    engine->audio_channel = NULL;
    
    /* Initialize state management */
    engine->state = TTS_STREAMING_STATE_IDLE;
    g_mutex_init(&engine->state_mutex);
    g_cond_init(&engine->state_cond);
    
    /* Initialize text queue */
    engine->text_queue = g_queue_new();
    g_mutex_init(&engine->queue_mutex);
    g_cond_init(&engine->queue_cond);
    engine->feeder_thread = NULL;
    engine->should_stop_feeding = false;
    
    /* Initialize audio management */
    engine->audio_thread = NULL;
    engine->should_stop_audio = false;
    
    /* Initialize engine configuration */
    engine->engine_type = engine_type;
    engine->speed = 1.0f;
    engine->volume = 80;
    engine->voice_name = NULL;
    
    /* Initialize callbacks */
    engine->segment_finished_callback = NULL;
    engine->state_changed_callback = NULL;
    engine->callback_user_data = NULL;
    
    girara_info("🔧 DEBUG: Created streaming TTS engine (type: %d)", engine_type);
    return engine;
}

void 
tts_streaming_engine_free(tts_streaming_engine_t* engine) 
{
    if (engine == NULL) {
        return;
    }
    
    girara_info("🔧 DEBUG: Freeing streaming TTS engine");
    
    /* Stop engine if active */
    tts_streaming_engine_stop(engine);
    
    /* Clean up text queue */
    g_mutex_lock(&engine->queue_mutex);
    while (!g_queue_is_empty(engine->text_queue)) {
        tts_text_segment_t* segment = g_queue_pop_head(engine->text_queue);
        tts_text_segment_free(segment);
    }
    g_queue_free(engine->text_queue);
    g_mutex_unlock(&engine->queue_mutex);
    
    /* Clean up synchronization primitives */
    g_mutex_clear(&engine->state_mutex);
    g_cond_clear(&engine->state_cond);
    g_mutex_clear(&engine->queue_mutex);
    g_cond_clear(&engine->queue_cond);
    
    /* Clean up configuration */
    g_free(engine->voice_name);
    
    g_free(engine);
}

/* Engine control */

bool 
tts_streaming_engine_start(tts_streaming_engine_t* engine) 
{
    if (engine == NULL) {
        return false;
    }
    
    g_mutex_lock(&engine->state_mutex);
    
    if (engine->state != TTS_STREAMING_STATE_IDLE) {
        g_mutex_unlock(&engine->state_mutex);
        return false;
    }
    
    girara_info("🔧 DEBUG: Starting streaming TTS engine");
    
    /* Set starting state */
    tts_streaming_engine_set_state(engine, TTS_STREAMING_STATE_STARTING);
    
    /* Spawn TTS process */
    if (!tts_streaming_engine_spawn_process(engine)) {
        girara_error("Failed to spawn TTS process");
        tts_streaming_engine_set_state(engine, TTS_STREAMING_STATE_ERROR);
        g_mutex_unlock(&engine->state_mutex);
        return false;
    }
    
    /* Start feeder thread */
    engine->should_stop_feeding = false;
    engine->feeder_thread = g_thread_new("tts-feeder", tts_text_feeder_thread, engine);
    
    /* Start audio thread */
    engine->should_stop_audio = false;
    engine->audio_thread = g_thread_new("tts-audio", tts_audio_player_thread, engine);
    
    /* Set active state */
    tts_streaming_engine_set_state(engine, TTS_STREAMING_STATE_ACTIVE);
    
    g_mutex_unlock(&engine->state_mutex);
    
    girara_info("✅ DEBUG: Streaming TTS engine started successfully");
    return true;
}

bool 
tts_streaming_engine_stop(tts_streaming_engine_t* engine) 
{
    if (engine == NULL) {
        return false;
    }
    
    g_mutex_lock(&engine->state_mutex);
    
    if (engine->state == TTS_STREAMING_STATE_IDLE) {
        g_mutex_unlock(&engine->state_mutex);
        return true;
    }
    
    girara_info("🔧 DEBUG: Stopping streaming TTS engine");
    
    /* Set stopping state */
    tts_streaming_engine_set_state(engine, TTS_STREAMING_STATE_STOPPING);
    
    /* Signal threads to stop */
    engine->should_stop_feeding = true;
    engine->should_stop_audio = true;
    
    /* Wake up threads */
    g_cond_broadcast(&engine->queue_cond);
    
    g_mutex_unlock(&engine->state_mutex);
    
    /* Wait for threads to finish */
    if (engine->feeder_thread != NULL) {
        g_thread_unref(engine->feeder_thread);
        engine->feeder_thread = NULL;
    }
    
    if (engine->audio_thread != NULL) {
        g_thread_unref(engine->audio_thread);
        engine->audio_thread = NULL;
    }
    
    /* Clean up process */
    tts_streaming_engine_cleanup_process(engine);
    
    /* Clear text queue */
    tts_streaming_engine_clear_queue(engine);
    
    /* Set idle state */
    g_mutex_lock(&engine->state_mutex);
    tts_streaming_engine_set_state(engine, TTS_STREAMING_STATE_IDLE);
    g_mutex_unlock(&engine->state_mutex);
    
    girara_info("✅ DEBUG: Streaming TTS engine stopped");
    return true;
}

/* Text management */

bool 
tts_streaming_engine_queue_segment(tts_streaming_engine_t* engine, tts_text_segment_t* segment) 
{
    if (engine == NULL || segment == NULL) {
        return false;
    }
    
    g_mutex_lock(&engine->queue_mutex);
    g_queue_push_tail(engine->text_queue, segment);
    size_t queue_size = g_queue_get_length(engine->text_queue);
    g_cond_signal(&engine->queue_cond);
    g_mutex_unlock(&engine->queue_mutex);
    
    girara_info("🔧 DEBUG: Queued text segment %d (queue size: %zu): '%.50s%s'", 
                 segment->segment_id, queue_size, segment->text, strlen(segment->text) > 50 ? "..." : "");
    
    return true;
}

bool 
tts_streaming_engine_queue_text(tts_streaming_engine_t* engine, const char* text, int segment_id) 
{
    if (engine == NULL || text == NULL) {
        return false;
    }
    
    /* Create a simple text segment for streaming */
    zathura_rectangle_t bounds = {0, 0, 0, 0}; /* Dummy bounds for streaming */
    tts_text_segment_t* segment = tts_text_segment_new(text, bounds, 0, segment_id, TTS_CONTENT_NORMAL);
    if (segment == NULL) {
        return false;
    }
    
    return tts_streaming_engine_queue_segment(engine, segment);
}

bool 
tts_streaming_engine_clear_queue(tts_streaming_engine_t* engine) 
{
    if (engine == NULL) {
        return false;
    }
    
    g_mutex_lock(&engine->queue_mutex);
    while (!g_queue_is_empty(engine->text_queue)) {
        tts_text_segment_t* segment = g_queue_pop_head(engine->text_queue);
        if (segment != NULL) {
            tts_text_segment_free(segment);
        }
    }
    g_mutex_unlock(&engine->queue_mutex);
    
    return true;
}

/* Internal implementation */

static bool 
tts_streaming_engine_spawn_process(tts_streaming_engine_t* engine) 
{
    if (engine == NULL) {
        return false;
    }
    
    /* Create pipes for communication */
    int stdin_pipe[2], stdout_pipe[2];
    
    if (pipe(stdin_pipe) == -1 || pipe(stdout_pipe) == -1) {
        girara_error("Failed to create pipes for TTS process");
        return false;
    }
    
    /* Build command based on engine type */
    char* command = NULL;
    switch (engine->engine_type) {
        case TTS_ENGINE_PIPER:
            /* Use Piper with streaming input - try Poetry first, then system piper */
            {
                char* current_dir = g_get_current_dir();
                char* project_dir = g_strdup_printf("%s/zathura-tts", current_dir);
                g_free(current_dir);
                
                /* Check if we're in a Poetry environment */
                if (g_file_test(g_strdup_printf("%s/pyproject.toml", project_dir), G_FILE_TEST_EXISTS)) {
                    /* Use Poetry-managed Piper */
                    command = g_strdup_printf("sh -c \"cd '%s' && poetry run piper --model '/home/user/Projects/zathura/zathura-tts/voices/en_US-lessac-medium.onnx' --output-raw\"", project_dir);
                } else {
                    /* Try system-installed piper */
                    command = g_strdup("piper --model '/home/user/Projects/zathura/zathura-tts/voices/en_US-lessac-medium.onnx' --output-raw");
                }
                g_free(project_dir);
            }
            break;
        case TTS_ENGINE_ESPEAK:
            /* Use espeak-ng with streaming input */
            command = g_strdup_printf("espeak-ng -s %d -a %d --stdin", 
                                    (int)(engine->speed * 175), engine->volume);
            break;
        case TTS_ENGINE_SPEECH_DISPATCHER:
            /* Use spd-say with streaming */
            command = g_strdup("spd-say -r %d -i %d");
            break;
        default:
            girara_error("Unsupported streaming engine type: %d", engine->engine_type);
            close(stdin_pipe[0]); close(stdin_pipe[1]);
            close(stdout_pipe[0]); close(stdout_pipe[1]);
            return false;
    }
    
    girara_info("🔧 DEBUG: Spawning streaming TTS process: %s", command);
    
    /* Fork and exec */
    GPid pid = fork();
    if (pid == 0) {
        /* Child process */
        
        /* Set up pipes */
        dup2(stdin_pipe[0], STDIN_FILENO);
        dup2(stdout_pipe[1], STDOUT_FILENO);
        
        /* Close unused pipe ends */
        close(stdin_pipe[1]);
        close(stdout_pipe[0]);
        
        /* Execute command */
        execl("/bin/sh", "sh", "-c", command, NULL);
        _exit(1); /* Should not reach here */
    } else if (pid > 0) {
        /* Parent process */
        
        /* Close unused pipe ends */
        close(stdin_pipe[0]);
        close(stdout_pipe[1]);
        
        /* Store process info */
        engine->process_pid = pid;
        engine->stdin_fd = stdin_pipe[1];
        engine->stdout_fd = stdout_pipe[0];
        
        /* Create GIO channels for non-blocking I/O */
        engine->text_channel = g_io_channel_unix_new(engine->stdin_fd);
        engine->audio_channel = g_io_channel_unix_new(engine->stdout_fd);
        
        /* Set channels to non-blocking */
        g_io_channel_set_flags(engine->text_channel, G_IO_FLAG_NONBLOCK, NULL);
        g_io_channel_set_flags(engine->audio_channel, G_IO_FLAG_NONBLOCK, NULL);
        
        g_free(command);
        return true;
    } else {
        /* Fork failed */
        girara_error("Failed to fork TTS process");
        close(stdin_pipe[0]); close(stdin_pipe[1]);
        close(stdout_pipe[0]); close(stdout_pipe[1]);
        g_free(command);
        return false;
    }
}

static void 
tts_streaming_engine_cleanup_process(tts_streaming_engine_t* engine) 
{
    if (engine == NULL) {
        return;
    }
    
    /* Close channels */
    if (engine->text_channel != NULL) {
        g_io_channel_shutdown(engine->text_channel, TRUE, NULL);
        g_io_channel_unref(engine->text_channel);
        engine->text_channel = NULL;
    }
    
    if (engine->audio_channel != NULL) {
        g_io_channel_shutdown(engine->audio_channel, TRUE, NULL);
        g_io_channel_unref(engine->audio_channel);
        engine->audio_channel = NULL;
    }
    
    /* Close file descriptors */
    if (engine->stdin_fd >= 0) {
        close(engine->stdin_fd);
        engine->stdin_fd = -1;
    }
    
    if (engine->stdout_fd >= 0) {
        close(engine->stdout_fd);
        engine->stdout_fd = -1;
    }
    
    /* Terminate process */
    if (engine->process_pid > 0) {
        girara_info("🔧 DEBUG: Terminating TTS process PID: %d", engine->process_pid);
        
        /* Try SIGTERM first */
        if (kill(engine->process_pid, SIGTERM) == 0) {
            usleep(200000); /* Wait 200ms */
            
            /* Check if still running */
            if (kill(engine->process_pid, 0) == 0) {
                girara_info("🔧 DEBUG: Process still running, using SIGKILL");
                kill(engine->process_pid, SIGKILL);
            }
            
            waitpid(engine->process_pid, NULL, 0);
        }
        
        engine->process_pid = 0;
    }
}

/* Thread implementations */

static gpointer 
tts_text_feeder_thread(gpointer data) 
{
    tts_streaming_engine_t* engine = (tts_streaming_engine_t*)data;
    
    girara_info("🔧 DEBUG: Text feeder thread started");
    
    while (!engine->should_stop_feeding) {
        g_mutex_lock(&engine->queue_mutex);
        
        /* Wait for text segments */
        while (g_queue_is_empty(engine->text_queue) && !engine->should_stop_feeding) {
            g_cond_wait(&engine->queue_cond, &engine->queue_mutex);
        }
        
        if (engine->should_stop_feeding) {
            g_mutex_unlock(&engine->queue_mutex);
            break;
        }
        
        /* Get next segment */
        tts_text_segment_t* segment = g_queue_pop_head(engine->text_queue);
        size_t remaining_queue_size = g_queue_get_length(engine->text_queue);
        g_mutex_unlock(&engine->queue_mutex);
        
        girara_info("🔧 DEBUG: Text feeder got segment: %p (remaining in queue: %zu)", (void*)segment, remaining_queue_size);
        
        if (segment != NULL && segment->text != NULL) {
            /* Send text to TTS process */
            if (engine->text_channel != NULL && strlen(segment->text) > 0) {
                gsize bytes_written;
                GError* error = NULL;
                
                /* Write text followed by newline */
                GIOStatus status = g_io_channel_write_chars(engine->text_channel, 
                                                          segment->text, 
                                                          strlen(segment->text), 
                                                          &bytes_written, 
                                                          &error);
                
                if (status == G_IO_STATUS_NORMAL) {
                    g_io_channel_write_chars(engine->text_channel, "\n", 1, &bytes_written, NULL);
                    g_io_channel_flush(engine->text_channel, NULL);
                    
                    girara_info("✅ DEBUG: Fed text segment %d to TTS process: '%.30s%s'", 
                               segment->segment_id, segment->text, strlen(segment->text) > 30 ? "..." : "");
                } else {
                    girara_error("🚨 DEBUG: Failed to write text to TTS process: %s", 
                                 error ? error->message : "unknown error");
                    if (error) g_error_free(error);
                }
            } else if (strlen(segment->text) == 0) {
                girara_warning("🔧 DEBUG: Skipping empty text segment %d", segment->segment_id);
            }
            
            /* Free the segment safely */
            tts_text_segment_free(segment);
        } else {
            girara_warning("🚨 DEBUG: Got NULL or invalid segment from queue");
        }
    }
    
    girara_info("🔧 DEBUG: Text feeder thread exiting");
    return NULL;
}

static gpointer 
tts_audio_player_thread(gpointer data) 
{
    tts_streaming_engine_t* engine = (tts_streaming_engine_t*)data;
    
    girara_info("🔧 DEBUG: Audio player thread started");
    
    /* For now, we'll use a simple approach - pipe audio to aplay */
    /* In a more sophisticated implementation, we'd buffer and manage audio directly */
    
    if (engine->audio_channel != NULL) {
        /* Start aplay process to handle audio output */
        GPid aplay_pid;
        int aplay_stdin;
        
        char* aplay_cmd[] = {"aplay", "-r", "22050", "-f", "S16_LE", "-t", "raw", "-", NULL};
        
        if (g_spawn_async_with_pipes(NULL, aplay_cmd, NULL,
                                   G_SPAWN_SEARCH_PATH | G_SPAWN_DO_NOT_REAP_CHILD,
                                   NULL, NULL, &aplay_pid, &aplay_stdin, NULL, NULL, NULL)) {
            
            /* Pipe audio from TTS to aplay */
            char buffer[4096];
            gsize bytes_read;
            
            while (!engine->should_stop_audio) {
                GIOStatus status = g_io_channel_read_chars(engine->audio_channel, 
                                                         buffer, sizeof(buffer), 
                                                         &bytes_read, NULL);
                
                if (status == G_IO_STATUS_NORMAL && bytes_read > 0) {
                    write(aplay_stdin, buffer, bytes_read);
                } else if (status == G_IO_STATUS_EOF) {
                    break;
                } else if (status == G_IO_STATUS_AGAIN) {
                    usleep(10000); /* Wait 10ms */
                }
            }
            
            /* Clean up aplay */
            close(aplay_stdin);
            kill(aplay_pid, SIGTERM);
            waitpid(aplay_pid, NULL, 0);
        }
    }
    
    girara_info("🔧 DEBUG: Audio player thread exiting");
    return NULL;
}

/* Utility functions - use existing text extractor functions */

/* State management */

static bool 
tts_streaming_engine_set_state(tts_streaming_engine_t* engine, tts_streaming_state_t new_state) 
{
    if (engine == NULL) {
        return false;
    }
    
    tts_streaming_state_t old_state = engine->state;
    engine->state = new_state;
    
    /* Signal state change */
    g_cond_broadcast(&engine->state_cond);
    
    /* Call callback if set */
    if (engine->state_changed_callback != NULL) {
        engine->state_changed_callback(old_state, new_state, engine->callback_user_data);
    }
    
    return true;
}

tts_streaming_state_t 
tts_streaming_engine_get_state(tts_streaming_engine_t* engine) 
{
    if (engine == NULL) {
        return TTS_STREAMING_STATE_ERROR;
    }
    
    g_mutex_lock(&engine->state_mutex);
    tts_streaming_state_t state = engine->state;
    g_mutex_unlock(&engine->state_mutex);
    
    return state;
}

bool 
tts_streaming_engine_is_active(tts_streaming_engine_t* engine) 
{
    tts_streaming_state_t state = tts_streaming_engine_get_state(engine);
    return (state == TTS_STREAMING_STATE_ACTIVE || state == TTS_STREAMING_STATE_PAUSED);
}