#include <stdio.h>
#include <string.h>

int main() {
    const char* test_text = "Name\tAge\tCity\nJohn\t25\tNYC";
    
    printf("Character by character analysis:\n");
    for (int i = 0; test_text[i]; i++) {
        printf("Char %d: '%c' (ASCII %d)\n", i, test_text[i], (int)test_text[i]);
        if (test_text[i] == '\n') {
            printf("  -> Found newline!\n");
        }
        if (test_text[i] == '\t') {
            printf("  -> Found tab!\n");
        }
    }
    
    return 0;
}