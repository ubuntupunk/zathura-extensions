/**
 * @file plugin.h
 * @brief Main plugin interface and core structures for Zathura TTS plugin
 * @author Zathura TTS Development Team
 * @date 2024
 * @copyright MIT License
 * 
 * This file defines the main plugin interface, core data structures, and
 * primary functions for the Zathura TTS plugin. It serves as the entry point
 * for plugin registration and initialization with the Zathura PDF viewer.
 */

#ifndef ZATHURA_TTS_PLUGIN_H
#define ZATHURA_TTS_PLUGIN_H

#include <zathura/plugin-api.h>
#include <girara/types.h>
#include <glib.h>

#include "config.h"

/**
 * @defgroup Plugin Core Plugin Interface
 * @brief Core plugin functionality and integration with Zathura
 * @{
 */

/** Plugin metadata constants */
#define TTS_PLUGIN_NAME PLUGIN_NAME          /**< Plugin name from build config */
#define TTS_PLUGIN_VERSION PLUGIN_VERSION    /**< Plugin version from build config */
#define TTS_PLUGIN_API_VERSION PLUGIN_API_VERSION /**< Zathura API version compatibility */

/** Forward declarations for core components */
typedef struct tts_session_s tts_session_t;           /**< TTS session container */
typedef struct tts_config_s tts_config_t;             /**< Configuration manager */
typedef struct tts_engine_s tts_engine_t;             /**< TTS engine interface */
typedef struct tts_audio_controller_s tts_audio_controller_t; /**< Audio controller */
typedef struct tts_ui_controller_s tts_ui_controller_t;       /**< UI controller */

/**
 * @brief Main TTS session structure containing all plugin components
 * 
 * This structure serves as the central container for all TTS plugin components.
 * It maintains references to the configuration, TTS engine, audio controller,
 * UI controller, and Zathura integration objects.
 */
struct tts_session_s {
  tts_config_t* config;                    /**< Configuration manager instance */
  tts_engine_t* engine;                    /**< Current TTS engine instance */
  tts_audio_controller_t* audio_controller; /**< Audio playback controller */
  tts_ui_controller_t* ui_controller;      /**< UI integration controller */
  zathura_t* zathura;                      /**< Zathura instance reference */
  girara_session_t* girara_session;        /**< Girara UI session reference */
  bool active;                             /**< Session active state flag */
};

/**
 * @brief Plugin metadata and state structure
 * 
 * Contains plugin identification information, Zathura integration references,
 * and the main TTS session. This structure represents the plugin instance
 * and is used for plugin lifecycle management.
 */
typedef struct tts_plugin_s {
  char* name;                              /**< Plugin name string */
  char* version;                           /**< Plugin version string */
  zathura_t* zathura;                      /**< Zathura instance reference */
  tts_session_t* session;                  /**< Main TTS session */
  bool initialized;                        /**< Plugin initialization state */
} tts_plugin_t;

/**
 * @defgroup Plugin_Lifecycle Plugin Lifecycle Management
 * @brief Functions for plugin registration, initialization, and cleanup
 * @{
 */

/**
 * @brief Register the TTS plugin with Zathura
 * 
 * This function registers the TTS plugin with the Zathura PDF viewer,
 * setting up the basic plugin metadata and preparing for initialization.
 * This is the first function called by Zathura when loading the plugin.
 * 
 * @param zathura Pointer to the Zathura instance
 * @return ZATHURA_ERROR_OK on success, error code on failure
 * 
 * @note This function must be called before any other plugin functions
 * @see tts_plugin_init()
 * @since 1.0.0
 */
zathura_error_t tts_plugin_register(zathura_t* zathura);

/**
 * @brief Initialize the TTS plugin and all its components
 * 
 * Performs complete plugin initialization including:
 * - Configuration system setup
 * - TTS engine detection and initialization
 * - Audio controller creation
 * - UI integration and keyboard shortcut registration
 * - Error handling system setup
 * 
 * @param zathura Pointer to the Zathura instance
 * @return ZATHURA_ERROR_OK on success, error code on failure
 * 
 * @note Must be called after tts_plugin_register()
 * @see tts_plugin_register(), tts_plugin_cleanup()
 * @since 1.0.0
 */
bool tts_plugin_init(zathura_t* zathura);

/**
 * @brief Clean up plugin resources and shutdown
 * 
 * Performs complete plugin cleanup including:
 * - Stopping any active TTS playback
 * - Releasing audio resources
 * - Cleaning up TTS engines
 * - Freeing configuration data
 * - Removing UI integrations
 * 
 * This function is called when Zathura is shutting down or when the
 * plugin is being unloaded.
 * 
 * @note Safe to call multiple times
 * @see tts_plugin_init()
 * @since 1.0.0
 */
void tts_plugin_cleanup(void);

/** @} */ // Plugin_Lifecycle

/**
 * @defgroup Plugin_State Plugin State Management
 * @brief Functions for querying and validating plugin state
 * @{
 */

/**
 * @brief Check if the plugin is fully initialized
 * 
 * @return true if plugin is initialized and ready for use, false otherwise
 * 
 * @note This function is thread-safe
 * @see tts_plugin_init()
 * @since 1.0.0
 */
bool tts_plugin_is_initialized(void);

/**
 * @brief Validate the current plugin state and configuration
 * 
 * Performs comprehensive validation of:
 * - Plugin initialization state
 * - Component availability and health
 * - Configuration validity
 * - TTS engine availability
 * 
 * @return ZATHURA_ERROR_OK if state is valid, error code describing the issue
 * 
 * @note This function can be used for debugging and health checks
 * @since 1.0.0
 */
zathura_error_t tts_plugin_validate_state(void);

/**
 * @brief Get the current plugin instance
 * 
 * Returns a pointer to the current plugin instance, providing access to
 * plugin metadata and the main TTS session. This function is primarily
 * used internally by plugin components.
 * 
 * @return Pointer to plugin instance, or NULL if not initialized
 * 
 * @warning The returned pointer should not be modified directly
 * @note This function is thread-safe
 * @see tts_plugin_is_initialized()
 * @since 1.0.0
 */
tts_plugin_t* tts_plugin_get_instance(void);

/** @} */ // Plugin_State

/** @} */ // Plugin

#endif /* ZATHURA_TTS_PLUGIN_H */