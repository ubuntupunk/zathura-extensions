/* Zathura API Stubs
 * Temporary stub implementations for missing Zathura API functions
 * These should be replaced with proper implementations or removed when
 * the correct Zathura API is available
 */

#include <zathura/types.h>
#include <zathura/plugin-api.h>
#include <zathura/document.h>
#include <zathura/page.h>
#include <zathura/links.h>
#include <girara/types.h>
#include <girara/statusbar.h>

/* Stub implementations for missing Zathura API functions */

girara_session_t* 
zathura_get_session(zathura_t* zathura) 
{
    /* This is a stub - in real Zathura, this would return the actual session */
    (void)zathura;
    return NULL; /* Return NULL for now - tests should handle this gracefully */
}

zathura_document_t* 
zathura_get_document(zathura_t* zathura) 
{
    /* This is a stub - in real Zathura, this would return the current document */
    (void)zathura;
    return NULL; /* Return NULL for now - tests should handle this gracefully */
}

unsigned int 
zathura_document_get_current_page_number(zathura_document_t* document) 
{
    /* This is a stub - in real Zathura, this would return the current page number */
    (void)document;
    return 0; /* Return page 0 for now */
}

zathura_page_t* 
zathura_document_get_page(zathura_document_t* document, unsigned int page_number) 
{
    /* This is a stub - in real Zathura, this would return the specified page */
    (void)document;
    (void)page_number;
    return NULL; /* Return NULL for now - tests should handle this gracefully */
}

unsigned int 
zathura_document_get_number_of_pages(zathura_document_t* document) 
{
    /* This is a stub - in real Zathura, this would return the total page count */
    (void)document;
    return 1; /* Return 1 page for now */
}

zathura_link_type_t 
zathura_link_get_type(zathura_link_t* link) 
{
    /* This is a stub - in real Zathura, this would return the link type */
    (void)link;
    return ZATHURA_LINK_GOTO_DEST; /* Return a default link type */
}

zathura_link_target_t 
zathura_link_get_target(zathura_link_t* link) 
{
    /* This is a stub - in real Zathura, this would return the link target */
    (void)link;
    zathura_link_target_t target = {0}; /* Initialize to zero */
    return target;
}

/* Stub implementations for missing Girara API functions */

girara_statusbar_item_t* 
girara_statusbar_item_get_default(girara_session_t* session) 
{
    /* This is a stub - in real Girara, this would return the default status item */
    (void)session;
    return NULL; /* Return NULL for now - UI functions should handle this gracefully */
}

/* Page API stub implementations - needed for standalone compilation and testing
 * These functions are provided by Zathura at runtime when the plugin is loaded */

double 
zathura_page_get_width(zathura_page_t* page) 
{
    /* This is a stub - in real Zathura, this would return the actual page width */
    (void)page;
    return 595.0; /* A4 width in points for testing */
}

double 
zathura_page_get_height(zathura_page_t* page) 
{
    /* This is a stub - in real Zathura, this would return the actual page height */
    (void)page;
    return 842.0; /* A4 height in points for testing */
}

char* 
zathura_page_get_text(zathura_page_t* page, zathura_rectangle_t rectangle, zathura_error_t* error) 
{
    /* This is a stub - in real Zathura, this would extract text from the page */
    (void)page;
    (void)rectangle;
    if (error) {
        *error = ZATHURA_ERROR_OK;
    }
    return g_strdup("Sample text for testing purposes. This is a mock implementation of page text extraction.");
}

unsigned int 
zathura_page_get_index(zathura_page_t* page) 
{
    /* This is a stub - in real Zathura, this would return the page index */
    (void)page;
    return 0; /* Return first page index for testing */
}

girara_list_t* 
zathura_page_links_get(zathura_page_t* page, zathura_error_t* error) 
{
    /* This is a stub - in real Zathura, this would return the page links */
    (void)page;
    if (error) {
        *error = ZATHURA_ERROR_OK;
    }
    return girara_list_new2(g_free); /* Return empty list for testing */
}