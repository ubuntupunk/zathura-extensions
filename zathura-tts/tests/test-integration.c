/* Integration tests for TTS plugin */

#include "test-framework.h"
#include "../src/plugin.h"
#include "../src/tts-config.h"
#include "../src/tts-error.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Mock Zathura structures and functions for integration testing */
typedef struct {
    void* session;
    void* document;
} mock_zathura_t;

typedef struct {
    void* data;
} mock_girara_session_t;

static mock_zathura_t* mock_zathura = NULL;
static mock_girara_session_t* mock_session = NULL;

/* Mock Zathura functions */
mock_girara_session_t* zathura_get_session(void* zathura) {
    (void)zathura;
    return mock_session;
}

/* Test plugin registration and lifecycle */
static void
test_plugin_lifecycle(void)
{
    TEST_CASE_BEGIN("Plugin Lifecycle");
    
    /* Create mock Zathura instance */
    mock_zathura = malloc(sizeof(mock_zathura_t));
    mock_session = malloc(sizeof(mock_girara_session_t));
    TEST_ASSERT_NOT_NULL(mock_zathura, "Mock Zathura should be created");
    TEST_ASSERT_NOT_NULL(mock_session, "Mock session should be created");
    
    if (mock_zathura && mock_session) {
        mock_zathura->session = mock_session;
        mock_session->data = NULL;
        
        /* Test plugin registration */
        zathura_error_t result = tts_plugin_register((zathura_t*)mock_zathura);
        TEST_ASSERT_EQUAL(ZATHURA_ERROR_OK, result, "Plugin registration should succeed");
        
        /* Test plugin is not initialized yet */
        bool is_initialized = tts_plugin_is_initialized();
        TEST_ASSERT(!is_initialized, "Plugin should not be initialized after registration");
        
        /* Test plugin initialization */
        result = tts_plugin_init((zathura_t*)mock_zathura);
        TEST_ASSERT_EQUAL(ZATHURA_ERROR_OK, result, "Plugin initialization should succeed");
        
        /* Test plugin is now initialized */
        is_initialized = tts_plugin_is_initialized();
        TEST_ASSERT(is_initialized, "Plugin should be initialized after init");
        
        /* Test plugin state validation */
        result = tts_plugin_validate_state();
        TEST_ASSERT_EQUAL(ZATHURA_ERROR_OK, result, "Plugin state validation should succeed");
        
        /* Test plugin instance access */
        tts_plugin_t* instance = tts_plugin_get_instance();
        TEST_ASSERT_NOT_NULL(instance, "Plugin instance should be accessible");
        
        if (instance) {
            TEST_ASSERT_NOT_NULL(instance->name, "Plugin should have a name");
            TEST_ASSERT_NOT_NULL(instance->version, "Plugin should have a version");
            TEST_ASSERT_EQUAL((void*)mock_zathura, (void*)instance->zathura, "Plugin should reference correct Zathura instance");
            TEST_ASSERT(instance->initialized, "Plugin should be marked as initialized");
        }
        
        /* Test plugin cleanup */
        tts_plugin_cleanup();
        
        /* Test plugin is no longer initialized */
        is_initialized = tts_plugin_is_initialized();
        TEST_ASSERT(!is_initialized, "Plugin should not be initialized after cleanup");
        
        /* Test plugin instance is cleared */
        instance = tts_plugin_get_instance();
        TEST_ASSERT_NULL(instance, "Plugin instance should be NULL after cleanup");
    }
    
    /* Cleanup mock objects */
    free(mock_zathura);
    free(mock_session);
    mock_zathura = NULL;
    mock_session = NULL;
    
    TEST_CASE_END();
}

/* Test configuration persistence and loading */
static void
test_configuration_persistence(void)
{
    TEST_CASE_BEGIN("Configuration Persistence");
    
    /* Create test configuration */
    tts_config_t* config = tts_config_new();
    TEST_ASSERT_NOT_NULL(config, "Configuration should be created");
    
    if (config) {
        /* Set test values */
        bool result = tts_config_set_default_speed(config, 1.5f);
        TEST_ASSERT(result, "Setting speed should succeed");
        
        result = tts_config_set_default_volume(config, 75);
        TEST_ASSERT(result, "Setting volume should succeed");
        
        result = tts_config_set_preferred_voice(config, "test-voice");
        TEST_ASSERT(result, "Setting voice should succeed");
        
        /* Test configuration is marked as modified */
        bool is_modified = tts_config_is_modified(config);
        TEST_ASSERT(is_modified, "Configuration should be marked as modified");
        
        /* Create temporary config file path */
        char temp_path[] = "/tmp/tts-test-config-XXXXXX";
        int fd = mkstemp(temp_path);
        TEST_ASSERT(fd != -1, "Temporary file should be created");
        close(fd);
        
        /* Test saving configuration */
        result = tts_config_save_to_file(config, temp_path);
        TEST_ASSERT(result, "Configuration should be saved to file");
        
        /* Create new configuration and load from file */
        tts_config_t* loaded_config = tts_config_new();
        TEST_ASSERT_NOT_NULL(loaded_config, "New configuration should be created");
        
        if (loaded_config) {
            result = tts_config_load_from_file(loaded_config, temp_path);
            TEST_ASSERT(result, "Configuration should be loaded from file");
            
            /* Verify loaded values match saved values */
            float loaded_speed = tts_config_get_default_speed(loaded_config);
            TEST_ASSERT_EQUAL(1.5f, loaded_speed, "Loaded speed should match saved speed");
            
            int loaded_volume = tts_config_get_default_volume(loaded_config);
            TEST_ASSERT_EQUAL(75, loaded_volume, "Loaded volume should match saved volume");
            
            const char* loaded_voice = tts_config_get_preferred_voice(loaded_config);
            TEST_ASSERT_NOT_NULL(loaded_voice, "Loaded voice should not be NULL");
            if (loaded_voice) {
                TEST_ASSERT_STRING_EQUAL("test-voice", loaded_voice, "Loaded voice should match saved voice");
            }
            
            /* Test configuration is not marked as modified after loading */
            bool loaded_is_modified = tts_config_is_modified(loaded_config);
            TEST_ASSERT(!loaded_is_modified, "Loaded configuration should not be marked as modified");
            
            tts_config_free(loaded_config);
        }
        
        /* Cleanup temporary file */
        unlink(temp_path);
        
        tts_config_free(config);
    }
    
    TEST_CASE_END();
}

/* Test error handling integration */
static void
test_error_handling_integration(void)
{
    TEST_CASE_BEGIN("Error Handling Integration");
    
    /* Test error context creation and handling */
    tts_error_context_t* context = tts_error_context_new(
        TTS_ERROR_NO_ENGINE,
        TTS_ERROR_SEVERITY_ERROR,
        "Test error message",
        "Additional details",
        "test-component",
        "test_function",
        123
    );
    
    TEST_ASSERT_NOT_NULL(context, "Error context should be created");
    
    if (context) {
        TEST_ASSERT_EQUAL(TTS_ERROR_NO_ENGINE, context->error_code, "Error code should match");
        TEST_ASSERT_EQUAL(TTS_ERROR_SEVERITY_ERROR, context->severity, "Severity should match");
        TEST_ASSERT_NOT_NULL(context->message, "Message should not be NULL");
        TEST_ASSERT_NOT_NULL(context->details, "Details should not be NULL");
        TEST_ASSERT_NOT_NULL(context->component, "Component should not be NULL");
        TEST_ASSERT_NOT_NULL(context->function, "Function should not be NULL");
        TEST_ASSERT_EQUAL(123, context->line, "Line number should match");
        TEST_ASSERT_NOT_NULL(context->timestamp, "Timestamp should not be NULL");
        
        /* Test error message generation */
        char* user_message = tts_error_get_user_message(context->error_code, context->details);
        TEST_ASSERT_NOT_NULL(user_message, "User message should be generated");
        
        if (user_message) {
            TEST_ASSERT(strlen(user_message) > 0, "User message should not be empty");
            g_free(user_message);
        }
        
        /* Test error context copying */
        tts_error_context_t* copy = tts_error_context_copy(context);
        TEST_ASSERT_NOT_NULL(copy, "Error context copy should be created");
        
        if (copy) {
            TEST_ASSERT_EQUAL(context->error_code, copy->error_code, "Copied error code should match");
            TEST_ASSERT_EQUAL(context->severity, copy->severity, "Copied severity should match");
            TEST_ASSERT_EQUAL(context->line, copy->line, "Copied line should match");
            
            tts_error_context_free(copy);
        }
        
        tts_error_context_free(context);
    }
    
    /* Test graceful degradation helpers */
    bool should_retry = tts_error_should_retry(TTS_ERROR_ENGINE_SPEAK_FAILED);
    TEST_ASSERT(should_retry, "Engine speak failure should be retryable");
    
    should_retry = tts_error_should_retry(TTS_ERROR_OUT_OF_MEMORY);
    TEST_ASSERT(!should_retry, "Out of memory should not be retryable");
    
    bool is_recoverable = tts_error_is_recoverable(TTS_ERROR_NO_TEXT);
    TEST_ASSERT(is_recoverable, "No text error should be recoverable");
    
    is_recoverable = tts_error_is_recoverable(TTS_ERROR_OUT_OF_MEMORY);
    TEST_ASSERT(!is_recoverable, "Out of memory should not be recoverable");
    
    TEST_CASE_END();
}

/* Test component integration */
static void
test_component_integration(void)
{
    TEST_CASE_BEGIN("Component Integration");
    
    /* Test that all error codes have string representations */
    const char* error_strings[] = {
        tts_error_get_string(TTS_ERROR_OK),
        tts_error_get_string(TTS_ERROR_NO_ENGINE),
        tts_error_get_string(TTS_ERROR_NO_TEXT),
        tts_error_get_string(TTS_ERROR_INVALID_CONFIG),
        tts_error_get_string(TTS_ERROR_UNKNOWN)
    };
    
    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_NOT_NULL(error_strings[i], "Error string should not be NULL");
        TEST_ASSERT(strlen(error_strings[i]) > 0, "Error string should not be empty");
    }
    
    /* Test severity level strings */
    const char* severity_strings[] = {
        tts_error_get_severity_string(TTS_ERROR_SEVERITY_INFO),
        tts_error_get_severity_string(TTS_ERROR_SEVERITY_WARNING),
        tts_error_get_severity_string(TTS_ERROR_SEVERITY_ERROR),
        tts_error_get_severity_string(TTS_ERROR_SEVERITY_CRITICAL)
    };
    
    for (int i = 0; i < 4; i++) {
        TEST_ASSERT_NOT_NULL(severity_strings[i], "Severity string should not be NULL");
        TEST_ASSERT(strlen(severity_strings[i]) > 0, "Severity string should not be empty");
    }
    
    /* Test configuration validation integration */
    tts_config_t* config = tts_config_new();
    TEST_ASSERT_NOT_NULL(config, "Configuration should be created");
    
    if (config) {
        char* error_message = NULL;
        bool is_valid = tts_config_validate(config, &error_message);
        TEST_ASSERT(is_valid, "Default configuration should be valid");
        TEST_ASSERT_NULL(error_message, "Error message should be NULL for valid config");
        
        /* Test invalid configuration */
        tts_config_set_default_speed(config, 10.0f); /* Invalid speed */
        is_valid = tts_config_validate(config, &error_message);
        TEST_ASSERT(!is_valid, "Invalid configuration should not validate");
        TEST_ASSERT_NOT_NULL(error_message, "Error message should be provided for invalid config");
        
        if (error_message) {
            TEST_ASSERT(strlen(error_message) > 0, "Error message should not be empty");
            g_free(error_message);
        }
        
        tts_config_free(config);
    }
    
    TEST_CASE_END();
}

/* Run all integration tests */
void
run_integration_tests(void)
{
    TEST_SUITE_BEGIN("TTS Integration Tests");
    
    test_plugin_lifecycle();
    test_configuration_persistence();
    test_error_handling_integration();
    test_component_integration();
    
    TEST_SUITE_END();
}

/* Integration test main function */
int
main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;
    
    /* Initialize test framework */
    test_framework_init();
    
    /* Run integration test suite */
    run_integration_tests();
    
    /* Print summary and cleanup */
    test_framework_print_summary();
    test_framework_cleanup();
    
    /* Return appropriate exit code */
    return test_framework_all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}