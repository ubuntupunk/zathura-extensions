/* Main test runner for TTS unit tests */

#include "test-framework.h"
#include <glib.h>
#include <stdlib.h>

/* Test function declarations */
void run_audio_controller_tests(void);

/* Mock implementations for testing - only what we need */

int 
main(int argc, char* argv[]) 
{
    (void)argc;
    (void)argv;
    
    /* Initialize test framework */
    test_framework_init();
    
    /* Run test suites */
    run_audio_controller_tests();
    
    /* Print summary and cleanup */
    test_framework_print_summary();
    test_framework_cleanup();
    
    /* Return appropriate exit code */
    return test_framework_all_passed() ? EXIT_SUCCESS : EXIT_FAILURE;
}