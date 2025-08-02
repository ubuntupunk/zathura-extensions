/* TTS Text Extractor Implementation - Standalone Functions Only
 * Contains functions that don't depend on Zathura runtime
 */

#include "tts-text-extractor.h"
#include <string.h>
#include <ctype.h>
#include <regex.h>

/* Helper function to clean up extracted text */
static char* clean_extracted_text(const char* raw_text) {
    if (raw_text == NULL) {
        return NULL;
    }
    
    size_t len = strlen(raw_text);
    char* cleaned = g_malloc(len + 1);
    if (cleaned == NULL) {
        return NULL;
    }
    
    size_t write_pos = 0;
    bool prev_was_space = false;
    
    for (size_t i = 0; i < len; i++) {
        char c = raw_text[i];
        
        /* Replace multiple whitespace with single space */
        if (isspace(c)) {
            if (!prev_was_space) {
                cleaned[write_pos++] = ' ';
                prev_was_space = true;
            }
        } else {
            cleaned[write_pos++] = c;
            prev_was_space = false;
        }
    }
    
    /* Remove trailing space */
    if (write_pos > 0 && cleaned[write_pos - 1] == ' ') {
        write_pos--;
    }
    
    cleaned[write_pos] = '\0';
    return cleaned;
}

tts_text_segment_t* tts_text_segment_new(const char* text, zathura_rectangle_t bounds, 
                                         int page_number, int segment_id, tts_content_type_t type) {
    if (text == NULL) {
        return NULL;
    }
    
    tts_text_segment_t* segment = g_malloc0(sizeof(tts_text_segment_t));
    if (segment == NULL) {
        return NULL;
    }
    
    segment->text = g_strdup(text);
    if (segment->text == NULL) {
        g_free(segment);
        return NULL;
    }
    
    segment->bounds = bounds;
    segment->page_number = page_number;
    segment->segment_id = segment_id;
    segment->type = type;
    
    return segment;
}

void tts_text_segment_free(tts_text_segment_t* segment) {
    if (segment == NULL) {
        return;
    }
    
    g_free(segment->text);
    g_free(segment);
}

girara_list_t* tts_segment_text_into_sentences(const char* text, zathura_error_t* error) {
    if (text == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    girara_list_t* sentences = girara_list_new();
    if (sentences == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    girara_list_set_free_function(sentences, g_free);
    
    size_t text_len = strlen(text);
    if (text_len == 0) {
        if (error) *error = ZATHURA_ERROR_OK;
        return sentences;
    }
    
    const char* sentence_start = text;
    const char* current = text;
    
    while (*current != '\0') {
        /* Look for sentence endings */
        if (*current == '.' || *current == '!' || *current == '?') {
            /* Check if this is really a sentence end */
            const char* next = current + 1;
            
            /* Skip whitespace after punctuation */
            while (*next != '\0' && isspace(*next)) {
                next++;
            }
            
            /* If next character is uppercase or end of text, it's likely a sentence end */
            if (*next == '\0' || isupper(*next)) {
                /* Extract the sentence */
                size_t sentence_len = current - sentence_start + 1;
                char* sentence = g_malloc(sentence_len + 1);
                if (sentence != NULL) {
                    strncpy(sentence, sentence_start, sentence_len);
                    sentence[sentence_len] = '\0';
                    
                    /* Clean up the sentence */
                    char* cleaned_sentence = clean_extracted_text(sentence);
                    g_free(sentence);
                    
                    if (cleaned_sentence != NULL && strlen(cleaned_sentence) > 0) {
                        girara_list_append(sentences, cleaned_sentence);
                    } else {
                        g_free(cleaned_sentence);
                    }
                }
                
                /* Move to start of next sentence */
                sentence_start = next;
                current = next - 1; /* Will be incremented at end of loop */
            }
        }
        current++;
    }
    
    /* Add any remaining text as the last sentence */
    if (sentence_start < current) {
        size_t sentence_len = current - sentence_start;
        char* sentence = g_malloc(sentence_len + 1);
        if (sentence != NULL) {
            strncpy(sentence, sentence_start, sentence_len);
            sentence[sentence_len] = '\0';
            
            char* cleaned_sentence = clean_extracted_text(sentence);
            g_free(sentence);
            
            if (cleaned_sentence != NULL && strlen(cleaned_sentence) > 0) {
                girara_list_append(sentences, cleaned_sentence);
            } else {
                g_free(cleaned_sentence);
            }
        }
    }
    
    if (error) *error = ZATHURA_ERROR_OK;
    return sentences;
}

bool tts_text_contains_math(const char* text) {
    if (text == NULL) {
        return false;
    }
    
    /* Simple heuristics for mathematical content */
    const char* math_indicators[] = {
        "∫", "∑", "∏", "√", "∞", "≤", "≥", "≠", "≈", "±", "×", "÷",
        "α", "β", "γ", "δ", "ε", "θ", "λ", "μ", "π", "σ", "φ", "ψ", "ω",
        "Δ", "Σ", "Π", "Ω",
        NULL
    };
    
    /* Check for mathematical symbols */
    for (int i = 0; math_indicators[i] != NULL; i++) {
        if (strstr(text, math_indicators[i]) != NULL) {
            return true;
        }
    }
    
    /* Check for patterns like equations */
    if (strstr(text, " = ") != NULL || strstr(text, " + ") != NULL || 
        strstr(text, " - ") != NULL || strstr(text, " * ") != NULL) {
        /* Additional check: contains numbers or variables */
        for (const char* p = text; *p; p++) {
            if (isdigit(*p) || (*p >= 'a' && *p <= 'z' && 
                (p == text || !isalpha(*(p-1))) && 
                (*(p+1) == '\0' || !isalpha(*(p+1))))) {
                return true;
            }
        }
    }
    
    return false;
}

bool tts_text_is_table_content(const char* text) {
    if (text == NULL) {
        return false;
    }
    
    /* Simple heuristics for table content */
    int tab_count = 0;
    int pipe_count = 0;
    int number_count = 0;
    int word_count = 0;
    int long_space_sequences = 0; /* Sequences of 3+ spaces */
    
    const char* p = text;
    bool in_word = false;
    int consecutive_spaces = 0;
    
    while (*p) {
        if (*p == '\t') {
            tab_count++;
            in_word = false;
            consecutive_spaces = 0;
        } else if (*p == '|') {
            pipe_count++;
            in_word = false;
            consecutive_spaces = 0;
        } else if (isdigit(*p)) {
            number_count++;
            if (!in_word) {
                word_count++;
                in_word = true;
            }
            consecutive_spaces = 0;
        } else if (isalpha(*p)) {
            if (!in_word) {
                word_count++;
                in_word = true;
            }
            consecutive_spaces = 0;
        } else if (*p == ' ') {
            if (in_word) {
                in_word = false;
            }
            consecutive_spaces++;
            if (consecutive_spaces >= 3) {
                long_space_sequences++;
                consecutive_spaces = 0; /* Reset to avoid double counting */
            }
        } else if (isspace(*p)) {
            if (in_word) {
                in_word = false;
            }
            consecutive_spaces = 0;
        } else {
            in_word = false;
            consecutive_spaces = 0;
        }
        p++;
    }
    
    /* Heuristic: likely table if has explicit separators (tabs/pipes) or multiple long space sequences */
    return (tab_count >= 1 || pipe_count >= 2 || long_space_sequences >= 2) && word_count >= 2;
}

bool tts_text_contains_links(const char* text) {
    if (text == NULL) {
        return false;
    }
    
    /* Simple heuristics for hyperlinks */
    const char* link_indicators[] = {
        "http://", "https://", "www.", "ftp://", "mailto:",
        ".com", ".org", ".net", ".edu", ".gov",
        NULL
    };
    
    for (int i = 0; link_indicators[i] != NULL; i++) {
        if (strstr(text, link_indicators[i]) != NULL) {
            return true;
        }
    }
    
    return false;
}

char* tts_process_math_content(const char* text, zathura_error_t* error) {
    if (text == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    /* Create a processed version of mathematical text */
    size_t original_len = strlen(text);
    size_t buffer_size = original_len * 3; /* Allow for expansion */
    char* processed = g_malloc(buffer_size);
    if (processed == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    const char* src = text;
    char* dst = processed;
    size_t remaining = buffer_size - 1;
    
    /* Mathematical symbol replacements for better TTS pronunciation */
    struct {
        const char* symbol;
        const char* spoken;
    } math_replacements[] = {
        {"∫", " integral "},
        {"∑", " sum "},
        {"∏", " product "},
        {"√", " square root of "},
        {"∞", " infinity "},
        {"≤", " less than or equal to "},
        {"≥", " greater than or equal to "},
        {"≠", " not equal to "},
        {"≈", " approximately equal to "},
        {"±", " plus or minus "},
        {"×", " times "},
        {"÷", " divided by "},
        {"α", " alpha "},
        {"β", " beta "},
        {"γ", " gamma "},
        {"δ", " delta "},
        {"ε", " epsilon "},
        {"θ", " theta "},
        {"λ", " lambda "},
        {"μ", " mu "},
        {"π", " pi "},
        {"σ", " sigma "},
        {"φ", " phi "},
        {"ψ", " psi "},
        {"ω", " omega "},
        {"Δ", " Delta "},
        {"Σ", " Sigma "},
        {"Π", " Pi "},
        {"Ω", " Omega "},
        {NULL, NULL}
    };
    
    while (*src && remaining > 0) {
        bool replaced = false;
        
        /* Check for mathematical symbols */
        for (int i = 0; math_replacements[i].symbol != NULL; i++) {
            size_t symbol_len = strlen(math_replacements[i].symbol);
            if (strncmp(src, math_replacements[i].symbol, symbol_len) == 0) {
                size_t spoken_len = strlen(math_replacements[i].spoken);
                if (spoken_len < remaining) {
                    strcpy(dst, math_replacements[i].spoken);
                    dst += spoken_len;
                    remaining -= spoken_len;
                    src += symbol_len;
                    replaced = true;
                    break;
                }
            }
        }
        
        if (!replaced) {
            *dst++ = *src++;
            remaining--;
        }
    }
    
    *dst = '\0';
    
    if (error) *error = ZATHURA_ERROR_OK;
    return processed;
}

char* tts_process_table_content(const char* text, zathura_error_t* error) {
    if (text == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    /* Create a processed version with table structure announcements */
    size_t original_len = strlen(text);
    size_t buffer_size = original_len * 3; /* Allow for more expansion */
    char* processed = g_malloc(buffer_size);
    if (processed == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    const char* src = text;
    char* dst = processed;
    size_t remaining = buffer_size - 1;
    
    /* Add table announcement at the beginning */
    const char* table_start = "Table content: ";
    size_t start_len = strlen(table_start);
    if (start_len < remaining) {
        strcpy(dst, table_start);
        dst += start_len;
        remaining -= start_len;
    }
    
    bool first_column = true;
    
    while (*src && remaining > 0) {
        if (*src == '\t' || *src == '|') {
            /* Announce column separator */
            const char* separator = first_column ? "" : ", next column: ";
            size_t sep_len = strlen(separator);
            if (sep_len < remaining) {
                strncpy(dst, separator, sep_len);
                dst += sep_len;
                remaining -= sep_len;
            }
            first_column = false;
            src++;
        } else if (*src == '\n') {
            /* Announce row separator */
            const char* row_sep = ", next row: ";
            size_t row_len = strlen(row_sep);
            if (row_len < remaining) {
                strncpy(dst, row_sep, row_len);
                dst += row_len;
                remaining -= row_len;
            }
            first_column = true;
            src++;
        } else {
            *dst++ = *src++;
            remaining--;
        }
    }
    
    *dst = '\0';
    
    if (error) *error = ZATHURA_ERROR_OK;
    return processed;
}

char* tts_process_link_content(const char* text, zathura_error_t* error) {
    if (text == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    /* Create a processed version with link announcements */
    size_t original_len = strlen(text);
    size_t buffer_size = original_len * 2; /* Allow for expansion */
    char* processed = g_malloc(buffer_size);
    if (processed == NULL) {
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    const char* src = text;
    char* dst = processed;
    size_t remaining = buffer_size - 1;
    
    /* Link patterns to detect and announce */
    struct {
        const char* pattern;
        const char* announcement;
    } link_patterns[] = {
        {"http://", "Link: "},
        {"https://", "Secure link: "},
        {"www.", "Web link: "},
        {"ftp://", "FTP link: "},
        {"mailto:", "Email link: "},
        {NULL, NULL}
    };
    
    while (*src && remaining > 0) {
        bool found_link = false;
        
        /* Check for link patterns */
        for (int i = 0; link_patterns[i].pattern != NULL; i++) {
            size_t pattern_len = strlen(link_patterns[i].pattern);
            if (strncmp(src, link_patterns[i].pattern, pattern_len) == 0) {
                /* Add announcement */
                size_t announce_len = strlen(link_patterns[i].announcement);
                if (announce_len < remaining) {
                    strcpy(dst, link_patterns[i].announcement);
                    dst += announce_len;
                    remaining -= announce_len;
                }
                
                /* Copy the link itself */
                const char* link_start = src;
                while (*src && !isspace(*src) && remaining > 0) {
                    *dst++ = *src++;
                    remaining--;
                }
                
                /* Add closing announcement */
                const char* link_end = ", end link";
                size_t end_len = strlen(link_end);
                if (end_len < remaining) {
                    strcpy(dst, link_end);
                    dst += end_len;
                    remaining -= end_len;
                }
                
                found_link = true;
                break;
            }
        }
        
        if (!found_link) {
            *dst++ = *src++;
            remaining--;
        }
    }
    
    *dst = '\0';
    
    if (error) *error = ZATHURA_ERROR_OK;
    return processed;
}

tts_content_type_t tts_detect_content_type(const char* text) {
    if (text == NULL) {
        return TTS_CONTENT_NORMAL;
    }
    
    /* Check for mathematical content first */
    if (tts_text_contains_math(text)) {
        return TTS_CONTENT_FORMULA;
    }
    
    /* Check for table content */
    if (tts_text_is_table_content(text)) {
        return TTS_CONTENT_TABLE;
    }
    
    /* Check for links */
    if (tts_text_contains_links(text)) {
        return TTS_CONTENT_LINK;
    }
    
    /* Check for headings (simple heuristic) */
    size_t len = strlen(text);
    if (len < 100 && len > 5) {
        /* Short text that might be a heading */
        bool has_punctuation = false;
        for (const char* p = text; *p; p++) {
            if (*p == '.' || *p == '!' || *p == '?') {
                has_punctuation = true;
                break;
            }
        }
        if (!has_punctuation) {
            return TTS_CONTENT_HEADING;
        }
    }
    
    return TTS_CONTENT_NORMAL;
}

char* tts_process_text_segment(tts_text_segment_t* segment, zathura_error_t* error) {
    if (segment == NULL || segment->text == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    char* processed_text = NULL;
    
    switch (segment->type) {
        case TTS_CONTENT_FORMULA:
            processed_text = tts_process_math_content(segment->text, error);
            break;
            
        case TTS_CONTENT_TABLE:
            processed_text = tts_process_table_content(segment->text, error);
            break;
            
        case TTS_CONTENT_LINK:
            processed_text = tts_process_link_content(segment->text, error);
            break;
            
        case TTS_CONTENT_HEADING:
            /* Add heading announcement */
            {
                const char* heading_prefix = "Heading: ";
                size_t prefix_len = strlen(heading_prefix);
                size_t text_len = strlen(segment->text);
                processed_text = g_malloc(prefix_len + text_len + 1);
                if (processed_text != NULL) {
                    strcpy(processed_text, heading_prefix);
                    strcat(processed_text, segment->text);
                    if (error) *error = ZATHURA_ERROR_OK;
                } else {
                    if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
                }
            }
            break;
            
        case TTS_CONTENT_CAPTION:
            /* Add caption announcement */
            {
                const char* caption_prefix = "Image caption: ";
                size_t prefix_len = strlen(caption_prefix);
                size_t text_len = strlen(segment->text);
                processed_text = g_malloc(prefix_len + text_len + 1);
                if (processed_text != NULL) {
                    strcpy(processed_text, caption_prefix);
                    strcat(processed_text, segment->text);
                    if (error) *error = ZATHURA_ERROR_OK;
                } else {
                    if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
                }
            }
            break;
            
        case TTS_CONTENT_NORMAL:
        default:
            /* Return a copy of the original text */
            processed_text = g_strdup(segment->text);
            if (processed_text != NULL) {
                if (error) *error = ZATHURA_ERROR_OK;
            } else {
                if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
            }
            break;
    }
    
    return processed_text;
}