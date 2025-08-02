#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <glib.h>
#include <girara/datastructures.h>
#include <zathura/types.h>

/* Include the content types and function declarations */
typedef enum {
    TTS_CONTENT_NORMAL,
    TTS_CONTENT_HEADING,
    TTS_CONTENT_FORMULA,
    TTS_CONTENT_TABLE,
    TTS_CONTENT_LINK,
    TTS_CONTENT_CAPTION
} tts_content_type_t;

typedef struct {
    char* text;
    zathura_rectangle_t bounds;
    int page_number;
    int segment_id;
    tts_content_type_t type;
} tts_text_segment_t;

/* Function declarations */
extern char* tts_process_math_content(const char* text, zathura_error_t* error);
extern char* tts_process_table_content(const char* text, zathura_error_t* error);
extern char* tts_process_link_content(const char* text, zathura_error_t* error);
extern tts_content_type_t tts_detect_content_type(const char* text);
extern char* tts_process_text_segment(tts_text_segment_t* segment, zathura_error_t* error);
extern tts_text_segment_t* tts_text_segment_new(const char* text, zathura_rectangle_t bounds, 
                                                int page_number, int segment_id, tts_content_type_t type);
extern void tts_text_segment_free(tts_text_segment_t* segment);

/* Test functions */
void test_math_processing() {
    printf("Testing mathematical content processing...\n");
    
    const char* math_text = "The integral ∫ f(x) dx = π + α";
    zathura_error_t error;
    
    char* processed = tts_process_math_content(math_text, &error);
    
    assert(error == ZATHURA_ERROR_OK);
    assert(processed != NULL);
    
    printf("Original: '%s'\n", math_text);
    printf("Processed: '%s'\n", processed);
    
    /* Check that symbols were replaced */
    assert(strstr(processed, " integral ") != NULL);
    assert(strstr(processed, " pi ") != NULL);
    assert(strstr(processed, " alpha ") != NULL);
    
    g_free(processed);
    printf("✓ Math processing test passed\n");
}

void test_table_processing() {
    printf("Testing table content processing...\n");
    
    const char* table_text = "Name\tAge\tCity\nJohn\t25\tNYC";
    zathura_error_t error;
    
    char* processed = tts_process_table_content(table_text, &error);
    
    assert(error == ZATHURA_ERROR_OK);
    assert(processed != NULL);
    
    printf("Original: '%s'\n", table_text);
    printf("Processed: '%s'\n", processed);
    
    /* Check that table structure was announced */
    assert(strstr(processed, "Table content:") != NULL);
    assert(strstr(processed, "next column:") != NULL);
    assert(strstr(processed, "next row:") != NULL);
    
    g_free(processed);
    printf("✓ Table processing test passed\n");
}

void test_link_processing() {
    printf("Testing link content processing...\n");
    
    const char* link_text = "Visit https://example.com for more info";
    zathura_error_t error;
    
    char* processed = tts_process_link_content(link_text, &error);
    
    assert(error == ZATHURA_ERROR_OK);
    assert(processed != NULL);
    
    printf("Original: '%s'\n", link_text);
    printf("Processed: '%s'\n", processed);
    
    /* Check that link was announced */
    assert(strstr(processed, "Secure link:") != NULL);
    assert(strstr(processed, "end link") != NULL);
    
    g_free(processed);
    printf("✓ Link processing test passed\n");
}

void test_content_type_detection() {
    printf("Testing content type detection...\n");
    
    assert(tts_detect_content_type("The equation x = y + 2") == TTS_CONTENT_FORMULA);
    assert(tts_detect_content_type("Name\tAge\tCity") == TTS_CONTENT_TABLE);
    assert(tts_detect_content_type("Visit https://example.com") == TTS_CONTENT_LINK);
    assert(tts_detect_content_type("Chapter 1 Introduction") == TTS_CONTENT_HEADING);
    assert(tts_detect_content_type("This is a regular paragraph with punctuation.") == TTS_CONTENT_NORMAL);
    
    printf("✓ Content type detection test passed\n");
}

void test_text_segment_processing() {
    printf("Testing text segment processing...\n");
    
    zathura_rectangle_t bounds = {0.0, 0.0, 100.0, 20.0};
    zathura_error_t error;
    
    /* Test math segment */
    tts_text_segment_t* math_segment = tts_text_segment_new("∫ f(x) dx", bounds, 1, 0, TTS_CONTENT_FORMULA);
    char* processed_math = tts_process_text_segment(math_segment, &error);
    assert(error == ZATHURA_ERROR_OK);
    assert(processed_math != NULL);
    assert(strstr(processed_math, " integral ") != NULL);
    printf("Math segment: '%s'\n", processed_math);
    g_free(processed_math);
    tts_text_segment_free(math_segment);
    
    /* Test heading segment */
    tts_text_segment_t* heading_segment = tts_text_segment_new("Chapter 1", bounds, 1, 1, TTS_CONTENT_HEADING);
    char* processed_heading = tts_process_text_segment(heading_segment, &error);
    assert(error == ZATHURA_ERROR_OK);
    assert(processed_heading != NULL);
    assert(strstr(processed_heading, "Heading:") != NULL);
    printf("Heading segment: '%s'\n", processed_heading);
    g_free(processed_heading);
    tts_text_segment_free(heading_segment);
    
    /* Test table segment */
    tts_text_segment_t* table_segment = tts_text_segment_new("Name\tAge", bounds, 1, 2, TTS_CONTENT_TABLE);
    char* processed_table = tts_process_text_segment(table_segment, &error);
    assert(error == ZATHURA_ERROR_OK);
    assert(processed_table != NULL);
    assert(strstr(processed_table, "Table content:") != NULL);
    printf("Table segment: '%s'\n", processed_table);
    g_free(processed_table);
    tts_text_segment_free(table_segment);
    
    /* Test link segment */
    tts_text_segment_t* link_segment = tts_text_segment_new("https://example.com", bounds, 1, 3, TTS_CONTENT_LINK);
    char* processed_link = tts_process_text_segment(link_segment, &error);
    assert(error == ZATHURA_ERROR_OK);
    assert(processed_link != NULL);
    assert(strstr(processed_link, "Secure link:") != NULL);
    printf("Link segment: '%s'\n", processed_link);
    g_free(processed_link);
    tts_text_segment_free(link_segment);
    
    /* Test caption segment */
    tts_text_segment_t* caption_segment = tts_text_segment_new("Figure 1: Sample chart", bounds, 1, 4, TTS_CONTENT_CAPTION);
    char* processed_caption = tts_process_text_segment(caption_segment, &error);
    assert(error == ZATHURA_ERROR_OK);
    assert(processed_caption != NULL);
    assert(strstr(processed_caption, "Image caption:") != NULL);
    printf("Caption segment: '%s'\n", processed_caption);
    g_free(processed_caption);
    tts_text_segment_free(caption_segment);
    
    /* Test normal segment */
    tts_text_segment_t* normal_segment = tts_text_segment_new("Regular text.", bounds, 1, 5, TTS_CONTENT_NORMAL);
    char* processed_normal = tts_process_text_segment(normal_segment, &error);
    assert(error == ZATHURA_ERROR_OK);
    assert(processed_normal != NULL);
    assert(strcmp(processed_normal, "Regular text.") == 0);
    printf("Normal segment: '%s'\n", processed_normal);
    g_free(processed_normal);
    tts_text_segment_free(normal_segment);
    
    printf("✓ Text segment processing test passed\n");
}

int main() {
    printf("Running TTS Special Content Handling Tests\n");
    printf("==========================================\n");
    
    test_math_processing();
    test_table_processing();
    test_link_processing();
    test_content_type_detection();
    test_text_segment_processing();
    
    printf("\n✓ All special content handling tests passed!\n");
    return 0;
}