#include <stdio.h>
#include <zathura/types.h>

typedef enum {
    TTS_CONTENT_NORMAL,
    TTS_CONTENT_HEADING,
    TTS_CONTENT_FORMULA,
    TTS_CONTENT_TABLE,
    TTS_CONTENT_LINK,
    TTS_CONTENT_CAPTION
} tts_content_type_t;

extern tts_content_type_t tts_detect_content_type(const char* text);

const char* content_type_name(tts_content_type_t type) {
    switch (type) {
        case TTS_CONTENT_NORMAL: return "NORMAL";
        case TTS_CONTENT_HEADING: return "HEADING";
        case TTS_CONTENT_FORMULA: return "FORMULA";
        case TTS_CONTENT_TABLE: return "TABLE";
        case TTS_CONTENT_LINK: return "LINK";
        case TTS_CONTENT_CAPTION: return "CAPTION";
        default: return "UNKNOWN";
    }
}

int main() {
    const char* test_texts[] = {
        "The equation x = y + 2",
        "Name\tAge\tCity",
        "Visit https://example.com",
        "Chapter 1 Introduction",
        "This is a regular paragraph with punctuation.",
        NULL
    };
    
    for (int i = 0; test_texts[i] != NULL; i++) {
        tts_content_type_t type = tts_detect_content_type(test_texts[i]);
        printf("Text: '%s'\n", test_texts[i]);
        printf("Detected type: %s\n\n", content_type_name(type));
    }
    
    return 0;
}