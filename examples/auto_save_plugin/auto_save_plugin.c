/*
 * Auto-Save Circular Plugin for Azahar Emulator
 * Demonstrates C ABI plugin integration with savestates (slots 1 to 10 every 5 minutes).
 */

#include <stdio.h>
#include <string.h>
#include "common/plugin_api.h"

static const EmulatorAPI* g_api = NULL;
static int g_current_slot = 1;
static uint32_t g_elapsed_ms = 0;
static const uint32_t AUTO_SAVE_INTERVAL_MS = 5 * 60 * 1000; // 5 minutes = 300,000 ms

static const PluginInfo g_plugin_info = {
    .name = "Auto-Save Circular",
    .version = "1.0.0",
    .author = "Azahar Plugin Engine",
    .description = "Salva automaticamente a cada 5 minutos de forma circular nos slots 1 a 10.",
    .api_version = PLUGIN_API_VERSION
};

PLUGIN_EXPORT const PluginInfo* PLUGIN_CALL on_plugin_get_info(void) {
    return &g_plugin_info;
}

PLUGIN_EXPORT const PluginInfo* PLUGIN_CALL azahar_plugin_get_info(void) {
    return &g_plugin_info;
}

PLUGIN_EXPORT int PLUGIN_CALL on_plugin_init(const EmulatorAPI* api) {
    if (!api) {
        return -1;
    }
    g_api = api;
    g_elapsed_ms = 0;
    g_current_slot = 1;

    if (g_api->log_message) {
        g_api->log_message(PLUGIN_LOG_INFO, "Auto-Save Circular Plugin inicializado com sucesso (intervalo: 5 min, slots 1-10).");
    }
    return 0;
}

PLUGIN_EXPORT int PLUGIN_CALL azahar_plugin_init(const EmulatorAPI* api) {
    return on_plugin_init(api);
}

PLUGIN_EXPORT void PLUGIN_CALL on_frame_rendered(void) {
    // Optional per-frame hook
}

PLUGIN_EXPORT void PLUGIN_CALL azahar_plugin_frame(void) {
    on_frame_rendered();
}

PLUGIN_EXPORT void PLUGIN_CALL on_interval_tick(void) {
    if (!g_api) {
        return;
    }

    // Only count time while emulation is actively running
    if (g_api->get_emulation_state && g_api->get_emulation_state() == EMU_STATE_RUNNING) {
        g_elapsed_ms += 50; // Called approximately every 50ms

        if (g_elapsed_ms >= AUTO_SAVE_INTERVAL_MS) {
            g_elapsed_ms = 0;

            if (g_api->save_state_slot) {
                int res = g_api->save_state_slot(g_current_slot);
                char msg[128];
                if (res == 0) {
                    snprintf(msg, sizeof(msg), "[Auto-Save] Estado salvo automaticamente no slot %d com sucesso.", g_current_slot);
                    if (g_api->log_message) {
                        g_api->log_message(PLUGIN_LOG_INFO, msg);
                    }
                } else {
                    snprintf(msg, sizeof(msg), "[Auto-Save] Falha ao salvar no slot %d (codigo de erro: %d).", g_current_slot, res);
                    if (g_api->log_message) {
                        g_api->log_message(PLUGIN_LOG_WARN, msg);
                    }
                }

                // Advance circularly through slots 1 to 10
                g_current_slot = (g_current_slot % 10) + 1;
            }
        }
    }
}

PLUGIN_EXPORT void PLUGIN_CALL azahar_plugin_tick(void) {
    on_interval_tick();
}

PLUGIN_EXPORT void PLUGIN_CALL on_plugin_shutdown(void) {
    if (g_api && g_api->log_message) {
        g_api->log_message(PLUGIN_LOG_INFO, "Auto-Save Circular Plugin descarregado.");
    }
    g_api = NULL;
}

PLUGIN_EXPORT void PLUGIN_CALL azahar_plugin_shutdown(void) {
    on_plugin_shutdown();
}
