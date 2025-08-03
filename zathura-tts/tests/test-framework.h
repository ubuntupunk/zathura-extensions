#ifndef TEST_FRAMEWORK_H
#define TEST_FRAMEWORK_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

/* Simple test framework for TTS unit tests */

/* Test statistics */
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    int skipped_tests;
} test_stats_t;

/* Global test statistics */
extern test_stats_t g_test_stats;

/* Test macros */
#define TEST_ASSERT(condition, message) \
    do { \
        g_test_stats.total_tests++; \
        if (condition) { \
            g_test_stats.passed_tests++; \
            printf("  ✓ %s\n", message); \
        } else { \
            g_test_stats.failed_tests++; \
            printf("  ✗ %s (FAILED at %s:%d)\n", message, __FILE__, __LINE__); \
        } \
    } while (0)

#define TEST_ASSERT_EQUAL(expected, actual, message) \
    TEST_ASSERT((expected) == (actual), message)

#define TEST_ASSERT_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != NULL, message)

#define TEST_ASSERT_NULL(ptr, message) \
    TEST_ASSERT((ptr) == NULL, message)

#define TEST_ASSERT_STRING_EQUAL(expected, actual, message) \
    TEST_ASSERT(strcmp((expected), (actual)) == 0, message)

#define TEST_SKIP(message) \
    do { \
        g_test_stats.total_tests++; \
        g_test_stats.skipped_tests++; \
        printf("  - %s (SKIPPED)\n", message); \
    } while (0)

/* Test suite macros */
#define TEST_SUITE_BEGIN(name) \
    printf("\n=== Test Suite: %s ===\n", name)

#define TEST_SUITE_END() \
    printf("\n")

#define TEST_CASE_BEGIN(name) \
    printf("\nTest Case: %s\n", name)

#define TEST_CASE_END() \
    /* Nothing for now */

/* Test framework functions */
void test_framework_init(void);
void test_framework_cleanup(void);
void test_framework_print_summary(void);
bool test_framework_all_passed(void);

/* Mock helpers */
typedef struct {
    void* original_function;
    void* mock_function;
    bool is_mocked;
} mock_info_t;

void test_mock_function(void** original, void* mock);
void test_unmock_function(void** original, void* mock);

#endif /* TEST_FRAMEWORK_H */