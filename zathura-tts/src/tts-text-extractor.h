#ifndef TTS_TEXT_EXTRACTOR_H
#define TTS_TEXT_EXTRACTOR_H

#include <girara/datastructures.h>
#include <glib.h>
#include <stdbool.h>

/* Include Zathura types */
#include <zathura/types.h>

/* Forward declarations */
typedef struct zathura_page_s zathura_page_t;

/**
 * Content types for text segments
 */
typedef enum {
    TTS_CONTENT_NORMAL,    /**< Regular text content */
    TTS_CONTENT_HEADING,   /**< Heading text */
    TTS_CONTENT_FORMULA,   /**< Mathematical formula */
    TTS_CONTENT_TABLE,     /**< Table content */
    TTS_CONTENT_LINK,      /**< Hyperlink text */
    TTS_CONTENT_CAPTION    /**< Image/figure caption */
} tts_content_type_t;

/**
 * Text segment structure
 */
typedef struct {
    char* text;                    /**< The text content */
    zathura_rectangle_t bounds;    /**< Bounding rectangle of the text */
    int page_number;               /**< Page number containing this text */
    int segment_id;                /**< Unique segment identifier */
    tts_content_type_t type;       /**< Type of content */
} tts_text_segment_t;

/**
 * Extract all text from a page
 *
 * @param page The page to extract text from
 * @param error Set to an error value if an error occurred
 * @return The extracted text (needs to be freed with g_free) or NULL on error
 */
char* tts_extract_page_text(zathura_page_t* page, zathura_error_t* error);

/**
 * Extract text segments from a page for TTS reading
 *
 * @param page The page to extract text segments from
 * @param error Set to an error value if an error occurred
 * @return List of tts_text_segment_t* or NULL on error
 */
girara_list_t* tts_extract_text_segments(zathura_page_t* page, zathura_error_t* error);

/**
 * Create a new text segment
 *
 * @param text The text content (will be copied)
 * @param bounds The bounding rectangle
 * @param page_number The page number
 * @param segment_id The segment identifier
 * @param type The content type
 * @return New text segment or NULL on error
 */
tts_text_segment_t* tts_text_segment_new(const char* text, zathura_rectangle_t bounds, 
                                         int page_number, int segment_id, tts_content_type_t type);

/**
 * Free a text segment
 *
 * @param segment The segment to free
 */
void tts_text_segment_free(tts_text_segment_t* segment);

/**
 * Segment text into sentences for better TTS reading
 *
 * @param text The text to segment
 * @param error Set to an error value if an error occurred
 * @return List of sentence strings or NULL on error
 */
girara_list_t* tts_segment_text_into_sentences(const char* text, zathura_error_t* error);

/**
 * Check if text contains mathematical notation
 *
 * @param text The text to check
 * @return true if mathematical notation is detected
 */
bool tts_text_contains_math(const char* text);

/**
 * Check if text appears to be a table structure
 *
 * @param text The text to check
 * @return true if table structure is detected
 */
bool tts_text_is_table_content(const char* text);

/**
 * Check if text contains hyperlinks
 *
 * @param text The text to check
 * @return true if hyperlinks are detected
 */
bool tts_text_contains_links(const char* text);

/**
 * Process mathematical formulas for TTS reading
 *
 * @param text The text containing mathematical notation
 * @param error Set to an error value if an error occurred
 * @return Processed text suitable for TTS or NULL on error
 */
char* tts_process_math_content(const char* text, zathura_error_t* error);

/**
 * Process table content for TTS reading
 *
 * @param text The text containing table structure
 * @param error Set to an error value if an error occurred
 * @return Processed text with table structure announcements or NULL on error
 */
char* tts_process_table_content(const char* text, zathura_error_t* error);

/**
 * Process hyperlink content for TTS reading
 *
 * @param text The text containing hyperlinks
 * @param error Set to an error value if an error occurred
 * @return Processed text with link announcements or NULL on error
 */
char* tts_process_link_content(const char* text, zathura_error_t* error);

/**
 * Extract and process hyperlinks from a page
 *
 * @param page The page to extract links from
 * @param error Set to an error value if an error occurred
 * @return List of link descriptions for TTS or NULL on error
 */
girara_list_t* tts_extract_page_links(zathura_page_t* page, zathura_error_t* error);

/**
 * Detect and classify special content type
 *
 * @param text The text to analyze
 * @return The detected content type
 */
tts_content_type_t tts_detect_content_type(const char* text);

/**
 * Process text segment based on its content type
 *
 * @param segment The text segment to process
 * @param error Set to an error value if an error occurred
 * @return Processed text suitable for TTS or NULL on error
 */
char* tts_process_text_segment(tts_text_segment_t* segment, zathura_error_t* error);

#endif /* TTS_TEXT_EXTRACTOR_H */