#include <stdio.h>
#include <zathura/types.h>

extern bool tts_text_is_table_content(const char* text);

int main() {
    const char* test1 = "Name\tAge\tCity";
    const char* test2 = "John|25|NYC";
    const char* test3 = "Regular paragraph text";
    
    printf("Test 1: '%s' -> %s\n", test1, tts_text_is_table_content(test1) ? "true" : "false");
    printf("Test 2: '%s' -> %s\n", test2, tts_text_is_table_content(test2) ? "true" : "false");
    printf("Test 3: '%s' -> %s\n", test3, tts_text_is_table_content(test3) ? "true" : "false");
    
    return 0;
}