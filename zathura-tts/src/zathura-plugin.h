/* Zathura Plugin API
 * Minimal header for plugin development that only includes necessary functions
 */

#ifndef ZATHURA_PLUGIN_H
#define ZATHURA_PLUGIN_H

#include <zathura/types.h>
#include <girara/types.h>

/* Function to get the girara session from zathura instance
 * This is provided by Zathura at runtime */
girara_session_t* zathura_get_session(zathura_t* zathura);

/* Function to get the current document from zathura instance
 * This is provided by Zathura at runtime */
zathura_document_t* zathura_get_document(zathura_t* zathura);

#endif /* ZATHURA_PLUGIN_H */