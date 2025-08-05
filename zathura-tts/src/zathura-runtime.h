/* Zathura Runtime API
 * Functions that are provided by Zathura at runtime when the plugin is loaded
 * These are declared here but not implemented - Zathura provides them
 */

#ifndef ZATHURA_RUNTIME_H
#define ZATHURA_RUNTIME_H

#include <zathura/types.h>
#include <girara/types.h>

/* Function to get the girara session from zathura instance
 * This is provided by Zathura at runtime */
girara_session_t* zathura_get_session(zathura_t* zathura);

/* Function to get the current document from zathura instance
 * This is provided by Zathura at runtime */
zathura_document_t* zathura_get_document(zathura_t* zathura);

#endif /* ZATHURA_RUNTIME_H */