#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "src/tts-text-extractor.h"

/* Test helper functions */
void test_text_segment_creation() {
    printf("Testing text segment creation...\n");
    
    zathura_rectangle_t bounds = {0.0, 0.0, 100.0, 20.0};
    tts_text_segment_t* segment = tts_text_segment_new("Test text", bounds, 1, 0, TTS_CONTENT_NORMAL);
    
    assert(segment != NULL);
    assert(strcmp(segment->text, "Test text") == 0);
    assert(segment->page_number == 1);
    assert(segment->segment_id == 0);
    assert(segment->type == TTS_CONTENT_NORMAL);
    
    tts_text_segment_free(segment);
    printf("✓ Text segment creation test passed\n");
}

void test_sentence_segmentation() {
    printf("Testing sentence segmentation...\n");
    
    const char* test_text = "This is the first sentence. This is the second sentence! Is this the third sentence? Yes it is.";
    zathura_error_t error;
    
    girara_list_t* sentences = tts_segment_text_into_sentences(test_text, &error);
    
    assert(error == ZATHURA_ERROR_OK);
    assert(sentences != NULL);
    
    size_t count = girara_list_size(sentences);
    printf("Found %zu sentences\n", count);
    
    /* Print sentences for verification */
    girara_list_iterator_t* iter = girara_list_iterator(sentences);
    int i = 0;
    while (iter != NULL) {
        char* sentence = (char*)girara_list_iterator_data(iter);
        printf("Sentence %d: '%s'\n", i++, sentence);
        iter = girara_list_iterator_next(iter);
    }
    
    girara_list_free(sentences);
    printf("✓ Sentence segmentation test passed\n");
}

void test_math_detection() {
    printf("Testing math detection...\n");
    
    assert(tts_text_contains_math("The equation is x = y + 2") == true);
    assert(tts_text_contains_math("The integral ∫ f(x) dx") == true);
    assert(tts_text_contains_math("Regular text without math") == false);
    assert(tts_text_contains_math("α + β = γ") == true);
    
    printf("✓ Math detection test passed\n");
}

void test_table_detection() {
    printf("Testing table detection...\n");
    
    assert(tts_text_is_table_content("Name\tAge\tCity") == true);
    assert(tts_text_is_table_content("John|25|NYC") == true);
    assert(tts_text_is_table_content("Regular paragraph text") == false);
    
    printf("✓ Table detection test passed\n");
}

void test_link_detection() {
    printf("Testing link detection...\n");
    
    assert(tts_text_contains_links("Visit https://example.com") == true);
    assert(tts_text_contains_links("Check www.google.com") == true);
    assert(tts_text_contains_links("Email me at user@example.org") == true);
    assert(tts_text_contains_links("Regular text without links") == false);
    
    printf("✓ Link detection test passed\n");
}

int main() {
    printf("Running TTS Text Extractor Tests\n");
    printf("================================\n");
    
    test_text_segment_creation();
    test_sentence_segmentation();
    test_math_detection();
    test_table_detection();
    test_link_detection();
    
    printf("\n✓ All tests passed!\n");
    return 0;
}