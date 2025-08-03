/* Simple test framework implementation */

#include "test-framework.h"
#include <stdlib.h>

/* Global test statistics */
test_stats_t g_test_stats = {0, 0, 0, 0};

void 
test_framework_init(void) 
{
    g_test_stats.total_tests = 0;
    g_test_stats.passed_tests = 0;
    g_test_stats.failed_tests = 0;
    g_test_stats.skipped_tests = 0;
    
    printf("TTS Unit Test Framework Initialized\n");
    printf("====================================\n");
}

void 
test_framework_cleanup(void) 
{
    /* Nothing to cleanup for now */
}

void 
test_framework_print_summary(void) 
{
    printf("\n====================================\n");
    printf("Test Summary:\n");
    printf("  Total:   %d\n", g_test_stats.total_tests);
    printf("  Passed:  %d\n", g_test_stats.passed_tests);
    printf("  Failed:  %d\n", g_test_stats.failed_tests);
    printf("  Skipped: %d\n", g_test_stats.skipped_tests);
    
    if (g_test_stats.failed_tests == 0) {
        printf("\n🎉 All tests passed!\n");
    } else {
        printf("\n❌ %d test(s) failed.\n", g_test_stats.failed_tests);
    }
    printf("====================================\n");
}

bool 
test_framework_all_passed(void) 
{
    return g_test_stats.failed_tests == 0;
}

/* Simple mock system */
void 
test_mock_function(void** original, void* mock) 
{
    *original = mock;
}

void 
test_unmock_function(void** original, void* mock) 
{
    /* For now, just set to NULL - in a real implementation,
     * we would restore the original function */
    *original = NULL;
}