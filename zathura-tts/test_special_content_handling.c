#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

/* Mock Zathura types for testing */
typedef enum {
    ZATHURA_ERROR_OK,
    ZATHURA_ERROR_INVALID_ARGUMENTS,
    ZATHURA_ERROR_OUT_OF_MEMORY
} zathura_error_t;

/* Include our header */
#include "src/tts-text-extractor.h"

int main() {
    printf("Testing special content handling...\n");
    
    /* Test math detection */
    printf("Testing math detection:\n");
    assert(tts_text_contains_math("The integral ∫ f(x) dx") == true);
    assert(tts_text_contains_math("E = mc²") == true);
    assert(tts_text_contains_math("Regular text without math") == false);
    assert(tts_text_contains_math("x + y = z") == true);
    printf("✓ Math detection works\n");
    
    /* Test table detection */
    printf("Testing table detection:\n");
    assert(tts_text_is_table_content("Name\tAge\tCity") == true);
    assert(tts_text_is_table_content("John | 25 | NYC") == true);
    assert(tts_text_is_table_content("Regular paragraph text") == false);
    printf("✓ Table detection works\n");
    
    /* Test link detection */
    printf("Testing link detection:\n");
    assert(tts_text_contains_links("Visit https://example.com") == true);
    assert(tts_text_contains_links("Email me at user@example.com") == true);
    assert(tts_text_contains_links("Regular text without links") == false);
    printf("✓ Link detection works\n");
    
    /* Test content type detection */
    printf("Testing content type detection:\n");
    assert(tts_detect_content_type("∫ f(x) dx") == TTS_CONTENT_FORMULA);
    assert(tts_detect_content_type("Name\tAge\tCity") == TTS_CONTENT_TABLE);
    assert(tts_detect_content_type("Visit https://example.com") == TTS_CONTENT_LINK);
    assert(tts_detect_content_type("Regular text") == TTS_CONTENT_NORMAL);
    printf("✓ Content type detection works\n");
    
    /* Test math processing */
    printf("Testing math processing:\n");
    zathura_error_t error;
    char* processed = tts_process_math_content("∫ f(x) dx", &error);
    assert(processed != NULL);
    assert(strstr(processed, "integral") != NULL);
    g_free(processed);
    printf("✓ Math processing works\n");
    
    /* Test table processing */
    printf("Testing table processing:\n");
    processed = tts_process_table_content("Name\tAge\tCity", &error);
    assert(processed != NULL);
    assert(strstr(processed, "Table content") != NULL);
    g_free(processed);
    printf("✓ Table processing works\n");
    
    /* Test link processing */
    printf("Testing link processing:\n");
    processed = tts_process_link_content("Visit https://example.com for more info", &error);
    assert(processed != NULL);
    assert(strstr(processed, "link") != NULL);
    g_free(processed);
    printf("✓ Link processing works\n");
    
    printf("\nAll special content handling tests passed! ✓\n");
    return 0;
}