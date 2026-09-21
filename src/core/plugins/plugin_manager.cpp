// Copyright 2026 Citra Emulator Project / Azahar Emulator Project
// Licensed under GPLv2 or any later version
// Refer to the license.txt file included.

#include <chrono>
#include <filesystem>
#include <fstream>
#include <set>

#include "common/file_util.h"
#include "common/logging/log.h"
#include "core/core.h"
#include "core/plugins/plugin_manager.h"

namespace Plugins {

static int PluginSaveStateSlot(int slot_index) {
    return PluginManager::GetInstance().SaveStateSlot(slot_index);
}

static int PluginLoadStateSlot(int slot_index) {
    return PluginManager::GetInstance().LoadStateSlot(slot_index);
}

static void PluginLogMessage(int level, const char* message) {
    PluginManager::GetInstance().LogPluginMessage(level, message);
}

static int PluginGetEmulationState() {
    return static_cast<int>(PluginManager::GetInstance().GetEmulationState());
}

static int PluginGetCurrentSlot() {
    return PluginManager::GetInstance().GetCurrentSlot();
}

PluginManager& PluginManager::GetInstance() {
    static PluginManager instance;
    return instance;
}

PluginManager::PluginManager() {
    emulator_api.api_version = PLUGIN_API_VERSION;
    emulator_api.save_state_slot = PluginSaveStateSlot;
    emulator_api.load_state_slot = PluginLoadStateSlot;
    emulator_api.log_message = PluginLogMessage;
    emulator_api.get_emulation_state = PluginGetEmulationState;
    emulator_api.get_current_slot = PluginGetCurrentSlot;
}

PluginManager::~PluginManager() {
    Shutdown();
}

void PluginManager::Initialize() {
    std::lock_guard lock{mutex};
    if (initialized) {
        return;
    }

    LOG_INFO(Core, "Initializing PluginManager...");
    std::string dir = GetPluginsDirectory();
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);

    ScanPlugins();
    StartTimerThread();
    initialized = true;
}

void PluginManager::Shutdown() {
    StopTimerThread();

    std::lock_guard lock{mutex};
    LOG_INFO(Core, "Shutting down PluginManager...");
    for (auto& plugin : plugins) {
        UnloadPluginInternal(plugin);
    }
    plugins.clear();
    initialized = false;
}

std::string PluginManager::GetPluginsDirectory() const {
    std::string user_dir = FileUtil::GetUserPath(FileUtil::UserPath::UserDir);
    std::filesystem::path p = std::filesystem::path(user_dir) / "plugins";
    return p.string();
}

void PluginManager::StartTimerThread() {
    if (timer_running.exchange(true)) {
        return;
    }

    timer_thread = std::make_unique<std::thread>([this]() {
        while (timer_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            if (!timer_running) {
                break;
            }
            OnIntervalTick();
        }
    });
}

void PluginManager::StopTimerThread() {
    if (!timer_running.exchange(false)) {
        return;
    }

    if (timer_thread && timer_thread->joinable()) {
        timer_thread->join();
    }
    timer_thread.reset();
}

void PluginManager::LoadEnabledSettings() {
    std::set<std::string> enabled_names;
    std::filesystem::path path = std::filesystem::path(GetPluginsDirectory()) / "enabled_plugins.txt";
    std::ifstream file(path);
    if (file.is_open()) {
        std::string line;
        while (std::getline(file, line)) {
            while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) {
                line.pop_back();
            }
            if (!line.empty() && line[0] != '#') {
                enabled_names.insert(line);
            }
        }
    }

    for (auto& plugin : plugins) {
        plugin.descriptor.is_enabled = enabled_names.count(plugin.descriptor.filename) > 0;
    }
}

void PluginManager::SaveEnabledSettings() {
    std::filesystem::path path = std::filesystem::path(GetPluginsDirectory()) / "enabled_plugins.txt";
    std::ofstream file(path, std::ios::trunc);
    if (file.is_open()) {
        file << "# Azahar Enabled Plugins List\n";
        for (const auto& plugin : plugins) {
            if (plugin.descriptor.is_enabled) {
                file << plugin.descriptor.filename << "\n";
            }
        }
    }
}

void PluginManager::ScanPlugins() {
    std::lock_guard lock{mutex};

    std::vector<std::string> search_dirs;
    search_dirs.push_back(GetPluginsDirectory());

    // Also search local ./plugins directory if distinct
    std::error_code ec;
    std::filesystem::path local_plugins = std::filesystem::current_path(ec) / "plugins";
    if (!ec && local_plugins.string() != search_dirs.front()) {
        search_dirs.push_back(local_plugins.string());
    }

    std::vector<std::filesystem::path> found_libs;
    for (const auto& dir : search_dirs) {
        if (!std::filesystem::exists(dir, ec)) {
            continue;
        }

        for (const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if (!entry.is_regular_file(ec)) {
                continue;
            }
            auto ext = entry.path().extension().string();
#if defined(_WIN32)
            if (ext == ".dll") {
                found_libs.push_back(entry.path());
            }
#elif defined(__APPLE__)
            if (ext == ".dylib" || ext == ".so") {
                found_libs.push_back(entry.path());
            }
#else
            if (ext == ".so") {
                found_libs.push_back(entry.path());
            }
#endif
        }
    }

    // Unload removed plugins
    for (auto it = plugins.begin(); it != plugins.end();) {
        bool still_exists = false;
        for (const auto& found : found_libs) {
            if (it->descriptor.filepath == found.string()) {
                still_exists = true;
                break;
            }
        }
        if (!still_exists) {
            UnloadPluginInternal(*it);
            it = plugins.erase(it);
        } else {
            ++it;
        }
    }

    // Add newly found plugins
    for (const auto& found : found_libs) {
        bool already_present = false;
        for (const auto& existing : plugins) {
            if (existing.descriptor.filepath == found.string()) {
                already_present = true;
                break;
            }
        }

        if (!already_present) {
            LoadedPlugin new_plugin;
            new_plugin.descriptor.filename = found.filename().string();
            new_plugin.descriptor.filepath = found.string();
            new_plugin.descriptor.name = found.stem().string();
            new_plugin.descriptor.version = "1.0.0";
            new_plugin.descriptor.author = "Unknown";
            new_plugin.descriptor.description = "No description provided.";
            new_plugin.descriptor.is_enabled = false;
            new_plugin.descriptor.is_loaded = false;

            plugins.push_back(std::move(new_plugin));
        }
    }

    LoadEnabledSettings();

    // Load active enabled plugins
    for (auto& plugin : plugins) {
        if (plugin.descriptor.is_enabled && !plugin.descriptor.is_loaded) {
            LoadPluginInternal(plugin);
        }
    }
}

void PluginManager::ReloadPlugins() {
    std::lock_guard lock{mutex};
    LOG_INFO(Core, "Reloading all plugins...");
    for (auto& plugin : plugins) {
        UnloadPluginInternal(plugin);
    }
    ScanPlugins();
}

void PluginManager::LoadPluginInternal(LoadedPlugin& plugin) {
    if (plugin.descriptor.is_loaded) {
        return;
    }

    LOG_INFO(Core, "Loading plugin: {} ({})", plugin.descriptor.name, plugin.descriptor.filepath);
    plugin.library = std::make_unique<Common::DynamicLibrary>();
    if (!plugin.library->Load(plugin.descriptor.filepath)) {
        LOG_ERROR(Core, "Failed to load plugin library {}: {}", plugin.descriptor.filepath,
                  plugin.library->GetLoadError());
        plugin.library.reset();
        plugin.descriptor.is_loaded = false;
        return;
    }

    // Resolve symbols with multiple fallback conventions
    plugin.get_info = plugin.library->GetSymbol<FnPluginGetInfo>("on_plugin_get_info");
    if (!plugin.get_info) {
        plugin.get_info = plugin.library->GetSymbol<FnPluginGetInfo>("azahar_plugin_get_info");
    }
    if (!plugin.get_info) {
        plugin.get_info = plugin.library->GetSymbol<FnPluginGetInfo>("get_plugin_info");
    }

    plugin.init = plugin.library->GetSymbol<FnPluginInit>("on_plugin_init");
    if (!plugin.init) {
        plugin.init = plugin.library->GetSymbol<FnPluginInit>("azahar_plugin_init");
    }

    plugin.on_frame = plugin.library->GetSymbol<FnPluginFrameRendered>("on_frame_rendered");
    if (!plugin.on_frame) {
        plugin.on_frame = plugin.library->GetSymbol<FnPluginFrameRendered>("azahar_plugin_frame");
    }

    plugin.on_tick = plugin.library->GetSymbol<FnPluginIntervalTick>("on_interval_tick");
    if (!plugin.on_tick) {
        plugin.on_tick = plugin.library->GetSymbol<FnPluginIntervalTick>("azahar_plugin_tick");
    }

    plugin.on_shutdown = plugin.library->GetSymbol<FnPluginShutdown>("on_plugin_shutdown");
    if (!plugin.on_shutdown) {
        plugin.on_shutdown = plugin.library->GetSymbol<FnPluginShutdown>("azahar_plugin_shutdown");
    }

    if (plugin.get_info) {
        const PluginInfo* info = plugin.get_info();
        if (info) {
            if (info->name && info->name[0] != '\0') plugin.descriptor.name = info->name;
            if (info->version && info->version[0] != '\0') plugin.descriptor.version = info->version;
            if (info->author && info->author[0] != '\0') plugin.descriptor.author = info->author;
            if (info->description && info->description[0] != '\0') plugin.descriptor.description = info->description;
            plugin.descriptor.api_version = info->api_version;
        }
    }

    if (plugin.init) {
        int res = plugin.init(&emulator_api);
        if (res != 0) {
            LOG_ERROR(Core, "Plugin {} on_plugin_init failed with code: {}", plugin.descriptor.name, res);
            UnloadPluginInternal(plugin);
            return;
        }
    }

    plugin.descriptor.is_loaded = true;
    LOG_INFO(Core, "Plugin loaded successfully: {} v{} by {}", plugin.descriptor.name,
             plugin.descriptor.version, plugin.descriptor.author);
}

void PluginManager::UnloadPluginInternal(LoadedPlugin& plugin) {
    if (!plugin.descriptor.is_loaded) {
        return;
    }

    LOG_INFO(Core, "Unloading plugin: {}", plugin.descriptor.name);
    if (plugin.on_shutdown) {
        try {
            plugin.on_shutdown();
        } catch (...) {
            LOG_ERROR(Core, "Exception during shutdown of plugin {}", plugin.descriptor.name);
        }
    }

    plugin.get_info = nullptr;
    plugin.init = nullptr;
    plugin.on_frame = nullptr;
    plugin.on_tick = nullptr;
    plugin.on_shutdown = nullptr;

    plugin.library.reset();
    plugin.descriptor.is_loaded = false;
}

void PluginManager::SetPluginEnabled(const std::string& filename, bool enabled) {
    std::lock_guard lock{mutex};
    for (auto& plugin : plugins) {
        if (plugin.descriptor.filename == filename) {
            if (plugin.descriptor.is_enabled == enabled) {
                return;
            }
            plugin.descriptor.is_enabled = enabled;
            if (enabled) {
                LoadPluginInternal(plugin);
            } else {
                UnloadPluginInternal(plugin);
            }
            SaveEnabledSettings();
            return;
        }
    }
}

bool PluginManager::IsPluginEnabled(const std::string& filename) const {
    std::lock_guard lock{mutex};
    for (const auto& plugin : plugins) {
        if (plugin.descriptor.filename == filename) {
            return plugin.descriptor.is_enabled;
        }
    }
    return false;
}

std::vector<PluginDescriptor> PluginManager::GetPlugins() const {
    std::lock_guard lock{mutex};
    std::vector<PluginDescriptor> list;
    list.reserve(plugins.size());
    for (const auto& plugin : plugins) {
        list.push_back(plugin.descriptor);
    }
    return list;
}

void PluginManager::OnFrameRendered() {
    std::lock_guard lock{mutex};
    for (auto& plugin : plugins) {
        if (plugin.descriptor.is_enabled && plugin.descriptor.is_loaded && plugin.on_frame) {
            try {
                plugin.on_frame();
            } catch (...) {
                LOG_ERROR(Core, "Exception during on_frame_rendered in plugin {}", plugin.descriptor.name);
            }
        }
    }
}

void PluginManager::OnIntervalTick() {
    std::lock_guard lock{mutex};
    for (auto& plugin : plugins) {
        if (plugin.descriptor.is_enabled && plugin.descriptor.is_loaded && plugin.on_tick) {
            try {
                plugin.on_tick();
            } catch (...) {
                LOG_ERROR(Core, "Exception during on_interval_tick in plugin {}", plugin.descriptor.name);
            }
        }
    }
}

EmulationState PluginManager::GetEmulationState() const {
    if (!Core::System::GetInstance().IsPoweredOn()) {
        return EMU_STATE_STOPPED;
    }
    return emulation_state.load();
}

void PluginManager::SetEmulationState(EmulationState state) {
    emulation_state.store(state);
}

int PluginManager::SaveStateSlot(int slot_index) {
    if (slot_index < 0 || slot_index > 10) {
        LOG_ERROR(Core, "Plugin attempted save to invalid slot index: {}", slot_index);
        return -1;
    }

    auto& system = Core::System::GetInstance();
    if (!system.IsPoweredOn()) {
        LOG_WARNING(Core, "Plugin attempted save while emulation is not powered on");
        return -2;
    }

    bool res = system.SendSignal(Core::System::Signal::Save, static_cast<u32>(slot_index));
    system.frame_limiter.AdvanceFrame();
    return res ? 0 : -3;
}

int PluginManager::LoadStateSlot(int slot_index) {
    if (slot_index < 0 || slot_index > 10) {
        LOG_ERROR(Core, "Plugin attempted load from invalid slot index: {}", slot_index);
        return -1;
    }

    auto& system = Core::System::GetInstance();
    if (!system.IsPoweredOn()) {
        LOG_WARNING(Core, "Plugin attempted load while emulation is not powered on");
        return -2;
    }

    bool res = system.SendSignal(Core::System::Signal::Load, static_cast<u32>(slot_index));
    return res ? 0 : -3;
}

void PluginManager::LogPluginMessage(int level, const char* message) {
    if (!message) return;

    switch (level) {
    case PLUGIN_LOG_TRACE:
        LOG_TRACE(Core, "[Plugin] {}", message);
        break;
    case PLUGIN_LOG_DEBUG:
        LOG_DEBUG(Core, "[Plugin] {}", message);
        break;
    case PLUGIN_LOG_INFO:
        LOG_INFO(Core, "[Plugin] {}", message);
        break;
    case PLUGIN_LOG_WARN:
        LOG_WARNING(Core, "[Plugin] {}", message);
        break;
    case PLUGIN_LOG_ERROR:
    case PLUGIN_LOG_CRITICAL:
        LOG_ERROR(Core, "[Plugin] {}", message);
        break;
    default:
        LOG_INFO(Core, "[Plugin] {}", message);
        break;
    }
}

void PluginManager::SetCurrentSlot(int slot) {
    current_slot.store(slot);
}

int PluginManager::GetCurrentSlot() const {
    return current_slot.load();
}

} // namespace Plugins
