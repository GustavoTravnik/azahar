// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#pragma once

#include <atomic>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include "common/dynamic_library/dynamic_library.h"
#include "common/plugin_api.h"

namespace Core {
class System;
}

namespace Plugins {

struct PluginDescriptor {
    std::string filename;
    std::string filepath;
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    bool is_enabled{false};
    bool is_loaded{false};
    uint32_t api_version{0};
};

struct LoadedPlugin {
    PluginDescriptor descriptor;
    std::unique_ptr<Common::DynamicLibrary> library;

    FnPluginGetInfo get_info{nullptr};
    FnPluginInit init{nullptr};
    FnPluginFrameRendered on_frame{nullptr};
    FnPluginIntervalTick on_tick{nullptr};
    FnPluginShutdown on_shutdown{nullptr};
};

class PluginManager {
public:
    static PluginManager& GetInstance();

    PluginManager(const PluginManager&) = delete;
    PluginManager& operator=(const PluginManager&) = delete;

    /// Initialize the plugin manager and scan for plugins
    void Initialize();

    /// Shutdown the plugin manager, unloading all plugins
    void Shutdown();

    /// Returns the directory where plugins are stored
    [[nodiscard]] std::string GetPluginsDirectory() const;

    /// Scans the plugins directory and refreshes the plugin list
    void ScanPlugins();

    /// Reloads all plugins from disk
    void ReloadPlugins();

    /// Sets whether a specific plugin is enabled
    void SetPluginEnabled(const std::string& filename, bool enabled);

    /// Checks if a plugin is enabled
    [[nodiscard]] bool IsPluginEnabled(const std::string& filename) const;

    /// Returns the list of currently detected plugins
    [[nodiscard]] std::vector<PluginDescriptor> GetPlugins() const;

    /// Hook called when a video frame has finished rendering
    void OnFrameRendered();

    /// Hook called periodically (every ~50ms)
    void OnIntervalTick();

    /// Returns the current emulation state
    [[nodiscard]] EmulationState GetEmulationState() const;

    /// Sets the current emulation state
    void SetEmulationState(EmulationState state);

    /// Save state in slot via API
    int SaveStateSlot(int slot_index);

    /// Load state from slot via API
    int LoadStateSlot(int slot_index);

    /// Log message from plugin
    void LogPluginMessage(int level, const char* message);

    /// Sets the current savestate slot index
    void SetCurrentSlot(int slot);

    /// Gets the current savestate slot index
    [[nodiscard]] int GetCurrentSlot() const;

private:
    PluginManager();
    ~PluginManager();

    void StartTimerThread();
    void StopTimerThread();

    void LoadPluginInternal(LoadedPlugin& plugin);
    void UnloadPluginInternal(LoadedPlugin& plugin);

    void LoadEnabledSettings();
    void SaveEnabledSettings();

    mutable std::recursive_mutex mutex;
    std::vector<LoadedPlugin> plugins;

    std::atomic<EmulationState> emulation_state{EMU_STATE_STOPPED};
    std::atomic<int> current_slot{1};

    std::atomic<bool> timer_running{false};
    std::unique_ptr<std::thread> timer_thread;

    EmulatorAPI emulator_api{};
    bool initialized{false};
};

} // namespace Plugins
