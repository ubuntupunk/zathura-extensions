/* Zathura API Stubs Header
 * Declarations for stub implementations of missing Zathura API functions
 */

#ifndef ZATHURA_STUBS_H
#define ZATHURA_STUBS_H

#include <zathura/types.h>
#include <zathura/plugin-api.h>
#include <zathura/document.h>
#include <zathura/page.h>
#include <zathura/links.h>
#include <girara/types.h>
#include <girara/statusbar.h>

/* Stub function declarations */

girara_session_t* zathura_get_session(zathura_t* zathura);
zathura_document_t* zathura_get_document(zathura_t* zathura);
unsigned int zathura_document_get_current_page_number(zathura_document_t* document);
zathura_page_t* zathura_document_get_page(zathura_document_t* document, unsigned int page_number);
unsigned int zathura_document_get_number_of_pages(zathura_document_t* document);
zathura_link_type_t zathura_link_get_type(zathura_link_t* link);
zathura_link_target_t zathura_link_get_target(zathura_link_t* link);
girara_statusbar_item_t* girara_statusbar_item_get_default(girara_session_t* session);

/* Page API stubs */
double zathura_page_get_width(zathura_page_t* page);
double zathura_page_get_height(zathura_page_t* page);
char* zathura_page_get_text(zathura_page_t* page, zathura_rectangle_t rectangle, zathura_error_t* error);
unsigned int zathura_page_get_index(zathura_page_t* page);
girara_list_t* zathura_page_links_get(zathura_page_t* page, zathura_error_t* error);

#endif /* ZATHURA_STUBS_H */