/* TTS Text Extractor Implementation
 * Extracts text from PDF pages for TTS reading
 */

#include "tts-text-extractor.h"
#include <string.h>
#include <ctype.h>
#include <regex.h>

/* Include Zathura headers */
#include <zathura/page.h>
#include <zathura/types.h>
#include <zathura/links.h>
#include "zathura-stubs.h"

/* Helper function to create a full page rectangle */
static zathura_rectangle_t get_full_page_rectangle(zathura_page_t* page) {
    zathura_rectangle_t rect = {0};
    
    if (page == NULL) {
        return rect;
    }
    
    rect.x1 = 0.0;
    rect.y1 = 0.0;
    rect.x2 = zathura_page_get_width(page);
    rect.y2 = zathura_page_get_height(page);
    
    return rect;
}

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

char* tts_extract_page_text(zathura_page_t* page, zathura_error_t* error) {
    if (page == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    /* Get the full page rectangle */
    zathura_rectangle_t full_page = get_full_page_rectangle(page);
    
    /* Extract text from the entire page */
    zathura_error_t local_error = ZATHURA_ERROR_OK;
    char* raw_text = zathura_page_get_text(page, full_page, &local_error);
    
    if (local_error != ZATHURA_ERROR_OK) {
        if (error) *error = local_error;
        return NULL;
    }
    
    if (raw_text == NULL) {
        if (error) *error = ZATHURA_ERROR_OK; /* No text is not an error */
        return NULL;
    }
    
    /* Clean up the extracted text */
    char* cleaned_text = clean_extracted_text(raw_text);
    g_free(raw_text);
    
    if (error) *error = ZATHURA_ERROR_OK;
    return cleaned_text;
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

girara_list_t* tts_extract_text_segments(zathura_page_t* page, zathura_error_t* error) {
    if (page == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    /* Extract the full page text first */
    zathura_error_t local_error = ZATHURA_ERROR_OK;
    char* page_text = tts_extract_page_text(page, &local_error);
    
    if (local_error != ZATHURA_ERROR_OK) {
        if (error) *error = local_error;
        return NULL;
    }
    
    if (page_text == NULL) {
        if (error) *error = ZATHURA_ERROR_OK;
        return NULL;
    }
    
    /* Create segments list */
    girara_list_t* segments = girara_list_new();
    if (segments == NULL) {
        g_free(page_text);
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    girara_list_set_free_function(segments, (girara_free_function_t)tts_text_segment_free);
    
    /* Segment the text into sentences */
    girara_list_t* sentences = tts_segment_text_into_sentences(page_text, &local_error);
    g_free(page_text);
    
    if (local_error != ZATHURA_ERROR_OK || sentences == NULL) {
        girara_list_free(segments);
        if (error) *error = local_error;
        return NULL;
    }
    
    /* Convert sentences to text segments */
    zathura_rectangle_t page_bounds = get_full_page_rectangle(page);
    int page_number = zathura_page_get_index(page);
    int segment_id = 0;
    
    girara_list_iterator_t* iter = girara_list_iterator(sentences);
    while (iter != NULL) {
        char* sentence = (char*)girara_list_iterator_data(iter);
        if (sentence != NULL) {
            /* Determine content type */
            tts_content_type_t content_type = TTS_CONTENT_NORMAL;
            if (tts_text_contains_math(sentence)) {
                content_type = TTS_CONTENT_FORMULA;
            } else if (tts_text_is_table_content(sentence)) {
                content_type = TTS_CONTENT_TABLE;
            } else if (tts_text_contains_links(sentence)) {
                content_type = TTS_CONTENT_LINK;
            }
            
            /* Create text segment */
            tts_text_segment_t* segment = tts_text_segment_new(sentence, page_bounds, 
                                                              page_number, segment_id++, content_type);
            if (segment != NULL) {
                girara_list_append(segments, segment);
            }
        }
        iter = girara_list_iterator_next(iter);
    }
    
    girara_list_free(sentences);
    
    if (error) *error = ZATHURA_ERROR_OK;
    return segments;
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
    
    const char* p = text;
    bool in_word = false;
    
    while (*p) {
        if (*p == '\t') {
            tab_count++;
        } else if (*p == '|') {
            pipe_count++;
        } else if (isdigit(*p)) {
            number_count++;
        } else if (isalpha(*p)) {
            if (!in_word) {
                word_count++;
                in_word = true;
            }
        } else if (isspace(*p)) {
            in_word = false;
        }
        p++;
    }
    
    /* Heuristic: likely table if has multiple tabs/pipes and mix of numbers/words */
    return (tab_count >= 2 || pipe_count >= 2) && number_count > 0 && word_count > 0;
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
    size_t buffer_size = original_len * 2; /* Allow for expansion */
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
    
    int column = 0;
    bool first_column = true;
    
    while (*src && remaining > 0) {
        if (*src == '\t' || *src == '|') {
            /* Announce column separator */
            const char* separator = first_column ? "" : ", next column: ";
            size_t sep_len = strlen(separator);
            if (sep_len < remaining) {
                strcpy(dst, separator);
                dst += sep_len;
                remaining -= sep_len;
                first_column = false;
            }
            src++;
        } else if (*src == '\n') {
            /* Announce row separator */
            const char* row_sep = ", next row: ";
            size_t row_len = strlen(row_sep);
            if (row_len < remaining) {
                strcpy(dst, row_sep);
                dst += row_len;
                remaining -= row_len;
                first_column = true;
            }
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

girara_list_t* tts_extract_page_links(zathura_page_t* page, zathura_error_t* error) {
    if (page == NULL) {
        if (error) *error = ZATHURA_ERROR_INVALID_ARGUMENTS;
        return NULL;
    }
    
    /* Get links from the page using Zathura's API */
    zathura_error_t local_error = ZATHURA_ERROR_OK;
    girara_list_t* page_links = zathura_page_links_get(page, &local_error);
    
    if (local_error != ZATHURA_ERROR_OK) {
        if (error) *error = local_error;
        return NULL;
    }
    
    if (page_links == NULL) {
        if (error) *error = ZATHURA_ERROR_OK;
        return NULL;
    }
    
    /* Create list for TTS-friendly link descriptions */
    girara_list_t* link_descriptions = girara_list_new();
    if (link_descriptions == NULL) {
        girara_list_free(page_links);
        if (error) *error = ZATHURA_ERROR_OUT_OF_MEMORY;
        return NULL;
    }
    
    girara_list_set_free_function(link_descriptions, g_free);
    
    /* Process each link */
    girara_list_iterator_t* iter = girara_list_iterator(page_links);
    while (iter != NULL) {
        zathura_link_t* link = (zathura_link_t*)girara_list_iterator_data(iter);
        if (link != NULL) {
            /* Get link type and target information */
            zathura_link_type_t link_type = zathura_link_get_type(link);
            zathura_link_target_t target = zathura_link_get_target(link);
            
            /* Create TTS-friendly description based on link type */
            char* link_desc = NULL;
            switch (link_type) {
                case ZATHURA_LINK_URI:
                    if (target.value != NULL) {
                        link_desc = g_strdup_printf("External link to %s", target.value);
                    } else {
                        link_desc = g_strdup("External link");
                    }
                    break;
                    
                case ZATHURA_LINK_GOTO_DEST:
                    link_desc = g_strdup_printf("Internal link to page %u", target.page_number);
                    break;
                    
                case ZATHURA_LINK_GOTO_REMOTE:
                    if (target.value != NULL) {
                        link_desc = g_strdup_printf("Link to external document %s, page %u", 
                                                  target.value, target.page_number);
                    } else {
                        link_desc = g_strdup_printf("Link to external document, page %u", target.page_number);
                    }
                    break;
                    
                case ZATHURA_LINK_LAUNCH:
                    if (target.value != NULL) {
                        link_desc = g_strdup_printf("Launch link to %s", target.value);
                    } else {
                        link_desc = g_strdup("Launch link");
                    }
                    break;
                    
                case ZATHURA_LINK_NAMED:
                    if (target.value != NULL) {
                        link_desc = g_strdup_printf("Named link %s", target.value);
                    } else {
                        link_desc = g_strdup("Named link");
                    }
                    break;
                    
                default:
                    link_desc = g_strdup("Link detected");
                    break;
            }
            
            if (link_desc != NULL) {
                girara_list_append(link_descriptions, link_desc);
            }
        }
        iter = girara_list_iterator_next(iter);
    }
    
    girara_list_free(page_links);
    
    if (error) *error = ZATHURA_ERROR_OK;
    return link_descriptions;
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