/* Unit tests for TTS Audio Controller */

#include "test-framework.h"
#include "../src/tts-audio-controller.h"
#include <glib.h>

/* Test audio controller creation and destruction */
static void
test_audio_controller_creation(void)
{
    TEST_CASE_BEGIN("Audio Controller Creation");
    
    /* Test creation */
    tts_audio_controller_t* controller = tts_audio_controller_new();
    TEST_ASSERT_NOT_NULL(controller, "Controller creation should succeed");
    
    if (controller != NULL) {
        /* Test initial state */
        tts_audio_state_t initial_state = tts_audio_controller_get_state(controller);
        TEST_ASSERT_EQUAL(TTS_AUDIO_STATE_STOPPED, initial_state, "Initial state should be STOPPED");
        
        /* Test initial position */
        int initial_page = tts_audio_controller_get_current_page(controller);
        int initial_segment = tts_audio_controller_get_current_segment(controller);
        TEST_ASSERT_EQUAL(-1, initial_page, "Initial page should be -1");
        TEST_ASSERT_EQUAL(-1, initial_segment, "Initial segment should be -1");
        
        /* Test initial audio settings */
        float initial_speed = tts_audio_controller_get_speed(controller);
        int initial_volume = tts_audio_controller_get_volume(controller);
        TEST_ASSERT_EQUAL(1.0f, initial_speed, "Initial speed should be 1.0");
        TEST_ASSERT_EQUAL(80, initial_volume, "Initial volume should be 80");
        
        /* Test cleanup */
        tts_audio_controller_free(controller);
    }
    
    /* Test NULL handling */
    tts_audio_controller_free(NULL); /* Should not crash */
    
    TEST_CASE_END();
}

/* Test audio state management */
static void
test_audio_state_management(void)
{
    TEST_CASE_BEGIN("Audio State Management");
    
    tts_audio_controller_t* controller = tts_audio_controller_new();
    TEST_ASSERT_NOT_NULL(controller, "Controller creation should succeed");
    
    if (controller != NULL) {
        /* Test valid state transitions */
        bool result = tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_PLAYING);
        TEST_ASSERT(result, "Transition from STOPPED to PLAYING should succeed");
        
        tts_audio_state_t current_state = tts_audio_controller_get_state(controller);
        TEST_ASSERT_EQUAL(TTS_AUDIO_STATE_PLAYING, current_state, "State should be PLAYING");
        
        result = tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_PAUSED);
        TEST_ASSERT(result, "Transition from PLAYING to PAUSED should succeed");
        
        current_state = tts_audio_controller_get_state(controller);
        TEST_ASSERT_EQUAL(TTS_AUDIO_STATE_PAUSED, current_state, "State should be PAUSED");
        
        result = tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_STOPPED);
        TEST_ASSERT(result, "Transition from PAUSED to STOPPED should succeed");
        
        /* Test invalid state transition */
        result = tts_audio_controller_set_state(controller, TTS_AUDIO_STATE_PAUSED);
        TEST_ASSERT(!result, "Invalid transition from STOPPED to PAUSED should fail");
        
        tts_audio_controller_free(controller);
    }
    
    TEST_CASE_END();
}

/* Test audio settings */
static void
test_audio_settings(void)
{
    TEST_CASE_BEGIN("Audio Settings");
    
    tts_audio_controller_t* controller = tts_audio_controller_new();
    TEST_ASSERT_NOT_NULL(controller, "Controller creation should succeed");
    
    if (controller != NULL) {
        /* Test speed settings */
        bool result = tts_audio_controller_set_speed(controller, 1.5f);
        TEST_ASSERT(result, "Setting valid speed should succeed");
        
        float speed = tts_audio_controller_get_speed(controller);
        TEST_ASSERT_EQUAL(1.5f, speed, "Speed should be 1.5");
        
        /* Test invalid speed */
        result = tts_audio_controller_set_speed(controller, 10.0f);
        TEST_ASSERT(!result, "Setting invalid speed should fail");
        
        speed = tts_audio_controller_get_speed(controller);
        TEST_ASSERT_EQUAL(1.5f, speed, "Speed should remain unchanged after invalid set");
        
        /* Test volume settings */
        result = tts_audio_controller_set_volume(controller, 50);
        TEST_ASSERT(result, "Setting valid volume should succeed");
        
        int volume = tts_audio_controller_get_volume(controller);
        TEST_ASSERT_EQUAL(50, volume, "Volume should be 50");
        
        /* Test invalid volume */
        result = tts_audio_controller_set_volume(controller, 150);
        TEST_ASSERT(!result, "Setting invalid volume should fail");
        
        volume = tts_audio_controller_get_volume(controller);
        TEST_ASSERT_EQUAL(50, volume, "Volume should remain unchanged after invalid set");
        
        tts_audio_controller_free(controller);
    }
    
    TEST_CASE_END();
}

/* Test session management */
static void
test_session_management(void)
{
    TEST_CASE_BEGIN("Session Management");
    
    tts_audio_controller_t* controller = tts_audio_controller_new();
    TEST_ASSERT_NOT_NULL(controller, "Controller creation should succeed");
    
    if (controller != NULL) {
        /* Create test segments */
        girara_list_t* segments = girara_list_new();
        TEST_ASSERT_NOT_NULL(segments, "Segments list creation should succeed");
        
        if (segments != NULL) {
            girara_list_set_free_function(segments, (girara_free_function_t)tts_text_segment_free);
            
            /* Add test segments */
            tts_text_segment_t* segment1 = tts_text_segment_new("Test text 1", 0, 0);
            tts_text_segment_t* segment2 = tts_text_segment_new("Test text 2", 0, 1);
            
            TEST_ASSERT_NOT_NULL(segment1, "Segment 1 creation should succeed");
            TEST_ASSERT_NOT_NULL(segment2, "Segment 2 creation should succeed");
            
            if (segment1 && segment2) {
                girara_list_append(segments, segment1);
                girara_list_append(segments, segment2);
                
                /* Test session start */
                bool result = tts_audio_controller_start_session(controller, segments);
                TEST_ASSERT(result, "Starting session should succeed");
                
                tts_audio_state_t state = tts_audio_controller_get_state(controller);
                TEST_ASSERT_EQUAL(TTS_AUDIO_STATE_PLAYING, state, "State should be PLAYING after session start");
                
                int current_page = tts_audio_controller_get_current_page(controller);
                int current_segment = tts_audio_controller_get_current_segment(controller);
                TEST_ASSERT_EQUAL(0, current_page, "Current page should be 0");
                TEST_ASSERT_EQUAL(0, current_segment, "Current segment should be 0");
                
                /* Test pause/resume */
                result = tts_audio_controller_pause_session(controller);
                TEST_ASSERT(result, "Pausing session should succeed");
                
                state = tts_audio_controller_get_state(controller);
                TEST_ASSERT_EQUAL(TTS_AUDIO_STATE_PAUSED, state, "State should be PAUSED");
                
                result = tts_audio_controller_resume_session(controller);
                TEST_ASSERT(result, "Resuming session should succeed");
                
                state = tts_audio_controller_get_state(controller);
                TEST_ASSERT_EQUAL(TTS_AUDIO_STATE_PLAYING, state, "State should be PLAYING after resume");
                
                /* Test stop */
                tts_audio_controller_stop_session(controller);
                state = tts_audio_controller_get_state(controller);
                TEST_ASSERT_EQUAL(TTS_AUDIO_STATE_STOPPED, state, "State should be STOPPED after stop");
            }
        }
        
        tts_audio_controller_free(controller);
    }
    
    TEST_CASE_END();
}

/* Test text segment helpers */
static void
test_text_segment_helpers(void)
{
    TEST_CASE_BEGIN("Text Segment Helpers");
    
    /* Test segment creation */
    tts_text_segment_t* segment = tts_text_segment_new("Test text", 5, 10);
    TEST_ASSERT_NOT_NULL(segment, "Segment creation should succeed");
    
    if (segment != NULL) {
        TEST_ASSERT_STRING_EQUAL("Test text", segment->text, "Segment text should match");
        TEST_ASSERT_EQUAL(5, segment->page_number, "Page number should be 5");
        TEST_ASSERT_EQUAL(10, segment->segment_id, "Segment ID should be 10");
        TEST_ASSERT_EQUAL(0, segment->start_offset, "Start offset should be 0");
        TEST_ASSERT_EQUAL(9, segment->end_offset, "End offset should be text length");
        
        tts_text_segment_free(segment);
    }
    
    /* Test NULL handling */
    tts_text_segment_t* null_segment = tts_text_segment_new(NULL, 0, 0);
    TEST_ASSERT_NULL(null_segment, "Creating segment with NULL text should fail");
    
    tts_text_segment_free(NULL); /* Should not crash */
    
    TEST_CASE_END();
}

/* Run all audio controller tests */
void
run_audio_controller_tests(void)
{
    TEST_SUITE_BEGIN("Audio Controller Tests");
    
    test_audio_controller_creation();
    test_audio_state_management();
    test_audio_settings();
    test_session_management();
    test_text_segment_helpers();
    
    TEST_SUITE_END();
}