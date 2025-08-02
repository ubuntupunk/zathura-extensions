#include <stdio.h>
#include <zathura/types.h>

extern char* tts_process_table_content(const char* text, zathura_error_t* error);

int main() {
    const char* test_text = "Name\tAge\tCity\nJohn\t25\tNYC";
    zathura_error_t error;
    
    printf("Input text (with escape sequences shown):\n");
    for (const char* p = test_text; *p; p++) {
        if (*p == '\t') {
            printf("\\t");
        } else if (*p == '\n') {
            printf("\\n");
        } else {
            printf("%c", *p);
        }
    }
    printf("\n\n");
    
    char* processed = tts_process_table_content(test_text, &error);
    printf("Processed: '%s'\n", processed);
    
    if (processed) {
        g_free(processed);
    }
    
    return 0;
}