/* Simple integration tests for TTS system components */

#include "test-framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Mock TTS configuration for integration testing */
typedef struct {
    float speed;
    int volume;
    bool valid;
} mock_config_t;

/* Mock TTS state for integration testing */
typedef enum {
    MOCK_STATE_STOPPED = 0,
    MOCK_STATE_PLAYING = 1,
    MOCK_STATE_PAUSED = 2,
    MOCK_STATE_ERROR = 3
} mock_state_t;

/* Mock audio controller for integration testing */
typedef struct {
    mock_state_t state;
    float speed;
    int volume;
    bool initialized;
} mock_controller_t;

/* Mock configuration functions */
mock_config_t* mock_config_new(void) {
    mock_config_t* config = malloc(sizeof(mock_config_t));
    if (config != NULL) {
        config->speed = 1.0f;
        config->volume = 80;
        config->valid = true;
    }
    return config;
}

void mock_config_free(mock_config_t* config) {
    free(config);
}

bool mock_config_set_speed(mock_config_t* config, float speed) {
    if (config == NULL || speed < 0.5f || speed > 3.0f) {
        return false;
    }
    config->speed = speed;
    return true;
}

bool mock_config_set_volume(mock_config_t* config, int volume) {
    if (config == NULL || volume < 0 || volume > 100) {
        return false;
    }
    config->volume = volume;
    return true;
}

/* Mock audio controller functions */
mock_controller_t* mock_controller_new(void) {
    mock_controller_t* controller = malloc(sizeof(mock_controller_t));
    if (controller != NULL) {
        controller->state = MOCK_STATE_STOPPED;
        controller->speed = 1.0f;
        controller->volume = 80;
        controller->initialized = true;
    }
    return controller;
}

void mock_controller_free(mock_controller_t* controller) {
    free(controller);
}

bool mock_controller_set_state(mock_controller_t* controller, mock_state_t new_state) {
    if (controller == NULL) {
        return false;
    }
    
    /* Validate state transitions */
    switch (controller->state) {
        case MOCK_STATE_STOPPED:
            if (new_state != MOCK_STATE_PLAYING && new_state != MOCK_STATE_ERROR) {
                return false;
            }
            break;
        case MOCK_STATE_PLAYING:
            if (new_state != MOCK_STATE_PAUSED && new_state != MOCK_STATE_STOPPED && new_state != MOCK_STATE_ERROR) {
                return false;
            }
            break;
        case MOCK_STATE_PAUSED:
            if (new_state != MOCK_STATE_PLAYING && new_state != MOCK_STATE_STOPPED && new_state != MOCK_STATE_ERROR) {
                return false;
            }
            break;
        case MOCK_STATE_ERROR:
            if (new_state != MOCK_STATE_STOPPED) {
                return false;
            }
            break;
    }
    
    controller->state = new_state;
    return true;
}

bool mock_controller_set_speed(mock_controller_t* controller, float speed) {
    if (controller == NULL || speed < 0.5f || speed > 3.0f) {
        return false;
    }
    controller->speed = speed;
    return true;
}

bool mock_controller_set_volume(mock_controller_t* controller, int volume) {
    if (controller == NULL || volume < 0 || volume > 100) {
        return false;
    }
    controller->volume = volume;
    return true;
}

/* Test configuration system integration */
static void
test_config_integration(void)
{
    TEST_CASE_BEGIN("Configuration System Integration");
    
    /* Test configuration creation and defaults */
    mock_config_t* config = mock_config_new();
    TEST_ASSERT_NOT_NULL(config, "Configuration should be created successfully");
    
    if (config != NULL) {
        /* Test default values */
        TEST_ASSERT_EQUAL(1.0f, config->speed, "Default speed should be 1.0");
        TEST_ASSERT_EQUAL(80, config->volume, "Default volume should be 80");
        TEST_ASSERT(config->valid, "Default configuration should be valid");
        
        /* Test configuration modification */
        bool speed_set = mock_config_set_speed(config, 1.5f);
        TEST_ASSERT(speed_set, "Setting valid speed should succeed");
        TEST_ASSERT_EQUAL(1.5f, config->speed, "Speed should be updated to 1.5");
        
        /* Test invalid configuration */
        bool invalid_speed = mock_config_set_speed(config, 10.0f);
        TEST_ASSERT(!invalid_speed, "Setting invalid speed should fail");
        TEST_ASSERT_EQUAL(1.5f, config->speed, "Speed should remain 1.5 after invalid set");
        
        /* Test volume configuration */
        bool volume_set = mock_config_set_volume(config, 90);
        TEST_ASSERT(volume_set, "Setting valid volume should succeed");
        TEST_ASSERT_EQUAL(90, config->volume, "Volume should be updated to 90");
        
        bool invalid_volume = mock_config_set_volume(config, 150);
        TEST_ASSERT(!invalid_volume, "Setting invalid volume should fail");
        TEST_ASSERT_EQUAL(90, config->volume, "Volume should remain 90 after invalid set");
        
        mock_config_free(config);
    }
    
    TEST_CASE_END();
}

/* Test audio controller integration */
static void
test_controller_integration(void)
{
    TEST_CASE_BEGIN("Audio Controller Integration");
    
    /* Create audio controller */
    mock_controller_t* controller = mock_controller_new();
    TEST_ASSERT_NOT_NULL(controller, "Audio controller should be created");
    
    if (controller != NULL) {
        /* Test initial state */
        TEST_ASSERT_EQUAL(MOCK_STATE_STOPPED, controller->state, "Initial state should be STOPPED");
        TEST_ASSERT(controller->initialized, "Controller should be initialized");
        
        /* Test settings integration */
        bool speed_set = mock_controller_set_speed(controller, 1.5f);
        TEST_ASSERT(speed_set, "Setting speed should succeed");
        TEST_ASSERT_EQUAL(1.5f, controller->speed, "Speed should be updated");
        
        bool volume_set = mock_controller_set_volume(controller, 90);
        TEST_ASSERT(volume_set, "Setting volume should succeed");
        TEST_ASSERT_EQUAL(90, controller->volume, "Volume should be updated");
        
        /* Test invalid settings */
        bool invalid_speed = mock_controller_set_speed(controller, 10.0f);
        TEST_ASSERT(!invalid_speed, "Setting invalid speed should fail");
        TEST_ASSERT_EQUAL(1.5f, controller->speed, "Speed should remain unchanged");
        
        /* Test state transitions */
        bool playing = mock_controller_set_state(controller, MOCK_STATE_PLAYING);
        TEST_ASSERT(playing, "Transition to PLAYING should succeed");
        TEST_ASSERT_EQUAL(MOCK_STATE_PLAYING, controller->state, "State should be PLAYING");
        
        bool paused = mock_controller_set_state(controller, MOCK_STATE_PAUSED);
        TEST_ASSERT(paused, "Transition to PAUSED should succeed");
        TEST_ASSERT_EQUAL(MOCK_STATE_PAUSED, controller->state, "State should be PAUSED");
        
        bool resumed = mock_controller_set_state(controller, MOCK_STATE_PLAYING);
        TEST_ASSERT(resumed, "Transition back to PLAYING should succeed");
        TEST_ASSERT_EQUAL(MOCK_STATE_PLAYING, controller->state, "State should be PLAYING");
        
        bool stopped = mock_controller_set_state(controller, MOCK_STATE_STOPPED);
        TEST_ASSERT(stopped, "Transition to STOPPED should succeed");
        TEST_ASSERT_EQUAL(MOCK_STATE_STOPPED, controller->state, "State should be STOPPED");
        
        /* Test invalid state transitions */
        bool invalid_transition = mock_controller_set_state(controller, MOCK_STATE_PAUSED);
        TEST_ASSERT(!invalid_transition, "Invalid transition from STOPPED to PAUSED should fail");
        TEST_ASSERT_EQUAL(MOCK_STATE_STOPPED, controller->state, "State should remain STOPPED");
        
        mock_controller_free(controller);
    }
    
    TEST_CASE_END();
}

/* Test complete TTS workflow integration */
static void
test_complete_workflow_integration(void)
{
    TEST_CASE_BEGIN("Complete TTS Workflow Integration");
    
    /* 1. Create and configure TTS system */
    mock_config_t* config = mock_config_new();
    TEST_ASSERT_NOT_NULL(config, "Configuration should be created");
    
    if (config != NULL) {
        /* Set custom configuration */
        mock_config_set_speed(config, 1.2f);
        mock_config_set_volume(config, 85);
        
        TEST_ASSERT_EQUAL(1.2f, config->speed, "Configuration speed should be set");
        TEST_ASSERT_EQUAL(85, config->volume, "Configuration volume should be set");
        
        /* 2. Create audio controller with configuration */
        mock_controller_t* controller = mock_controller_new();
        TEST_ASSERT_NOT_NULL(controller, "Audio controller should be created");
        
        if (controller != NULL) {
            /* Apply configuration to audio controller */
            mock_controller_set_speed(controller, config->speed);
            mock_controller_set_volume(controller, config->volume);
            
            /* Verify configuration was applied */
            TEST_ASSERT_EQUAL(1.2f, controller->speed, "Configuration speed should be applied");
            TEST_ASSERT_EQUAL(85, controller->volume, "Configuration volume should be applied");
            
            /* 3. Test complete state workflow */
            TEST_ASSERT_EQUAL(MOCK_STATE_STOPPED, controller->state, "Should start in STOPPED state");
            
            /* Start playback */
            bool started = mock_controller_set_state(controller, MOCK_STATE_PLAYING);
            TEST_ASSERT(started, "Should be able to start playback");
            TEST_ASSERT_EQUAL(MOCK_STATE_PLAYING, controller->state, "Should be in PLAYING state");
            
            /* Pause playback */
            bool paused = mock_controller_set_state(controller, MOCK_STATE_PAUSED);
            TEST_ASSERT(paused, "Should be able to pause playback");
            TEST_ASSERT_EQUAL(MOCK_STATE_PAUSED, controller->state, "Should be in PAUSED state");
            
            /* Resume playback */
            bool resumed = mock_controller_set_state(controller, MOCK_STATE_PLAYING);
            TEST_ASSERT(resumed, "Should be able to resume playback");
            TEST_ASSERT_EQUAL(MOCK_STATE_PLAYING, controller->state, "Should be in PLAYING state");
            
            /* Stop playback */
            bool stopped = mock_controller_set_state(controller, MOCK_STATE_STOPPED);
            TEST_ASSERT(stopped, "Should be able to stop playback");
            TEST_ASSERT_EQUAL(MOCK_STATE_STOPPED, controller->state, "Should be in STOPPED state");
            
            /* 4. Test error handling integration */
            bool invalid_transition = mock_controller_set_state(controller, MOCK_STATE_PAUSED);
            TEST_ASSERT(!invalid_transition, "Invalid state transition should fail");
            TEST_ASSERT_EQUAL(MOCK_STATE_STOPPED, controller->state, "State should remain STOPPED after invalid transition");
            
            /* 5. Test configuration changes during operation */
            mock_controller_set_state(controller, MOCK_STATE_PLAYING);
            
            /* Change speed during playback */
            bool speed_changed = mock_controller_set_speed(controller, 2.0f);
            TEST_ASSERT(speed_changed, "Should be able to change speed during playback");
            TEST_ASSERT_EQUAL(2.0f, controller->speed, "Speed should be updated");
            TEST_ASSERT_EQUAL(MOCK_STATE_PLAYING, controller->state, "State should remain PLAYING");
            
            mock_controller_free(controller);
        }
        
        mock_config_free(config);
    }
    
    TEST_CASE_END();
}

/* Test memory management integration */
static void
test_memory_management_integration(void)
{
    TEST_CASE_BEGIN("Memory Management Integration");
    
    /* Test multiple component creation and cleanup */
    for (int i = 0; i < 5; i++) {
        mock_config_t* config = mock_config_new();
        TEST_ASSERT_NOT_NULL(config, "Configuration creation should succeed in loop");
        
        mock_controller_t* controller = mock_controller_new();
        TEST_ASSERT_NOT_NULL(controller, "Audio controller creation should succeed in loop");
        
        if (config != NULL && controller != NULL) {
            /* Use the components */
            float test_speed = 1.0f + (i * 0.1f);
            mock_config_set_speed(config, test_speed);
            mock_controller_set_speed(controller, config->speed);
            
            TEST_ASSERT_EQUAL(test_speed, controller->speed, "Speed should be set correctly");
            TEST_ASSERT(controller->speed > 0.9f && controller->speed < 1.6f, "Speed should be in valid range");
            
            /* Test state operations */
            mock_controller_set_state(controller, MOCK_STATE_PLAYING);
            TEST_ASSERT_EQUAL(MOCK_STATE_PLAYING, controller->state, "State should be set correctly");
            
            mock_controller_set_state(controller, MOCK_STATE_STOPPED);
            TEST_ASSERT_EQUAL(MOCK_STATE_STOPPED, controller->state, "State should be reset correctly");
        }
        
        /* Cleanup */
        mock_controller_free(controller);
        mock_config_free(config);
    }
    
    /* Test NULL handling */
    mock_config_free(NULL); /* Should not crash */
    mock_controller_free(NULL); /* Should not crash */
    
    /* Test resource cleanup verification */
    mock_config_t* config = mock_config_new();
    mock_controller_t* controller = mock_controller_new();
    
    if (config != NULL && controller != NULL) {
        /* Use resources */
        mock_config_set_speed(config, 1.5f);
        mock_config_set_volume(config, 75);
        mock_controller_set_speed(controller, config->speed);
        mock_controller_set_volume(controller, config->volume);
        
        /* Verify resources are properly set */
        TEST_ASSERT_EQUAL(1.5f, config->speed, "Config speed should be set");
        TEST_ASSERT_EQUAL(75, config->volume, "Config volume should be set");
        TEST_ASSERT_EQUAL(1.5f, controller->speed, "Controller speed should be set");
        TEST_ASSERT_EQUAL(75, controller->volume, "Controller volume should be set");
    }
    
    /* Cleanup should not cause issues */
    mock_config_free(config);
    mock_controller_free(controller);
    
    TEST_CASE_END();
}

/* Run all integration tests */
void
run_integration_tests(void)
{
    TEST_SUITE_BEGIN("TTS Integration Tests");
    
    test_config_integration();
    test_controller_integration();
    test_complete_workflow_integration();
    test_memory_management_integration();
    
    TEST_SUITE_END();
}

/* Integration test main function */
int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    printf("===========================================\n");
    printf("TTS Integration Test Suite\n");
    printf("===========================================\n");
    
    /* Initialize test framework */
    test_framework_init();
    
    /* Run integration test suite */
    run_integration_tests();
    
    /* Print summary and cleanup */
    printf("\n===========================================\n");
    printf("INTEGRATION TEST RESULTS\n");
    printf("===========================================\n");
    test_framework_print_summary();
    test_framework_cleanup();
    
    /* Return appropriate exit code */
    return test_framework_all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}