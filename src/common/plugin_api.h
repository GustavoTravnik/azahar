// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#ifndef PLUGIN_API_H
#define PLUGIN_API_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32)
    #define PLUGIN_EXPORT __declspec(dllexport)
    #define PLUGIN_CALL __cdecl
#elif defined(__GNUC__) || defined(__clang__)
    #define PLUGIN_EXPORT __attribute__((visibility("default")))
    #define PLUGIN_CALL
#else
    #define PLUGIN_EXPORT
    #define PLUGIN_CALL
#endif

#define PLUGIN_API_VERSION_MAJOR 1
#define PLUGIN_API_VERSION_MINOR 0
#define PLUGIN_API_VERSION_PATCH 0
#define PLUGIN_API_VERSION ((PLUGIN_API_VERSION_MAJOR << 16) | (PLUGIN_API_VERSION_MINOR << 8) | PLUGIN_API_VERSION_PATCH)

/**
 * Emulation state values.
 */
typedef enum EmulationState {
    EMU_STATE_STOPPED = 0,
    EMU_STATE_RUNNING = 1,
    EMU_STATE_PAUSED  = 2
} EmulationState;

/**
 * Log levels matching emulator severity.
 */
typedef enum PluginLogLevel {
    PLUGIN_LOG_TRACE = 0,
    PLUGIN_LOG_DEBUG = 1,
    PLUGIN_LOG_INFO  = 2,
    PLUGIN_LOG_WARN  = 3,
    PLUGIN_LOG_ERROR = 4,
    PLUGIN_LOG_CRITICAL = 5
} PluginLogLevel;

/**
 * Metadata descriptor provided by the plugin.
 */
typedef struct PluginInfo {
    const char* name;
    const char* version;
    const char* author;
    const char* description;
    uint32_t api_version;
} PluginInfo;

/**
 * API functions exposed by Azahar to plugins.
 */
typedef struct EmulatorAPI {
    uint32_t api_version;

    /**
     * Saves emulation state into the specified slot (1-10 or custom slot).
     * @param slot_index The 1-based or 0-based slot index.
     * @return 0 on success, negative value on failure.
     */
    int (PLUGIN_CALL *save_state_slot)(int slot_index);

    /**
     * Loads emulation state from the specified slot.
     * @param slot_index The slot index.
     * @return 0 on success, negative value on failure.
     */
    int (PLUGIN_CALL *load_state_slot)(int slot_index);

    /**
     * Emits a message into Azahar's logging framework.
     * @param level PluginLogLevel value.
     * @param message UTF-8 null-terminated string.
     */
    void (PLUGIN_CALL *log_message)(int level, const char* message);

    /**
     * Gets current emulation execution state.
     * @return EmulationState (EMU_STATE_STOPPED, EMU_STATE_RUNNING, EMU_STATE_PAUSED).
     */
    int (PLUGIN_CALL *get_emulation_state)(void);

    /**
     * Gets the currently selected savestate slot in the UI (if available).
     * @return Slot index.
     */
    int (PLUGIN_CALL *get_current_slot)(void);
} EmulatorAPI;

/*
 * Plugin exported lifecycle hooks.
 * A plugin can export these functions:
 *
 * PLUGIN_EXPORT const PluginInfo* PLUGIN_CALL on_plugin_get_info(void);
 * PLUGIN_EXPORT int PLUGIN_CALL on_plugin_init(const EmulatorAPI* api);
 * PLUGIN_EXPORT void PLUGIN_CALL on_frame_rendered(void);
 * PLUGIN_EXPORT void PLUGIN_CALL on_interval_tick(void);
 * PLUGIN_EXPORT void PLUGIN_CALL on_plugin_shutdown(void);
 */

typedef const PluginInfo* (PLUGIN_CALL *FnPluginGetInfo)(void);
typedef int (PLUGIN_CALL *FnPluginInit)(const EmulatorAPI* api);
typedef void (PLUGIN_CALL *FnPluginFrameRendered)(void);
typedef void (PLUGIN_CALL *FnPluginIntervalTick)(void);
typedef void (PLUGIN_CALL *FnPluginShutdown)(void);

#ifdef __cplusplus
}
#endif

#endif /* PLUGIN_API_H */
