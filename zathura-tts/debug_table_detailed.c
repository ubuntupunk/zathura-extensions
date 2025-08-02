#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <stdbool.h>

char* debug_process_table_content(const char* text) {
    if (text == NULL) {
        return NULL;
    }
    
    /* Create a processed version with table structure announcements */
    size_t original_len = strlen(text);
    size_t buffer_size = original_len * 3; /* Allow for more expansion */
    char* processed = g_malloc(buffer_size);
    if (processed == NULL) {
        return NULL;
    }
    
    const char* src = text;
    char* dst = processed;
    size_t remaining = buffer_size - 1;
    
    /* Add table announcement at the beginning */
    const char* table_start = "Table content: ";
    size_t start_len = strlen(table_start);
    if (start_len < remaining) {
        strncpy(dst, table_start, start_len);
        dst += start_len;
        remaining -= start_len;
    }
    
    bool first_column = true;
    
    printf("Processing character by character:\n");
    while (*src && remaining > 0) {
        printf("Processing char '%c' (ASCII %d)\n", *src, (int)*src);
        
        if (*src == '\t' || *src == '|') {
            printf("  -> Found column separator\n");
            /* Announce column separator */
            const char* separator = first_column ? "" : ", next column: ";
            size_t sep_len = strlen(separator);
            printf("  -> Adding separator: '%s'\n", separator);
            if (sep_len < remaining) {
                strncpy(dst, separator, sep_len);
                dst += sep_len;
                remaining -= sep_len;
            }
            first_column = false;
            src++;
        } else if (*src == '\n') {
            printf("  -> Found newline - adding row separator\n");
            /* Announce row separator */
            const char* row_sep = ", next row: ";
            size_t row_len = strlen(row_sep);
            printf("  -> Adding row separator: '%s'\n", row_sep);
            if (row_len < remaining) {
                strncpy(dst, row_sep, row_len);
                dst += row_len;
                remaining -= row_len;
            }
            first_column = true;
            src++;
        } else {
            printf("  -> Adding regular character\n");
            *dst++ = *src++;
            remaining--;
        }
    }
    
    *dst = '\0';
    
    return processed;
}

int main() {
    const char* test_text = "Name\tAge\tCity\nJohn\t25\tNYC";
    
    printf("Input: '%s'\n\n", test_text);
    
    char* processed = debug_process_table_content(test_text);
    printf("\nResult: '%s'\n", processed);
    
    if (processed) {
        g_free(processed);
    }
    
    return 0;
}