#pragma once

#include "core/Types.hpp"
#include "FileWatcher.hpp"
#include <filesystem>
#include <functional>

namespace Engine {

// Forward declaration
class Shader;

namespace fs = std::filesystem;

// Callback for shader reload events
using ShaderReloadCallback = std::function<void(Ref<Shader>, bool success)>;

// Manages hot-reloading of shaders
class ShaderHotReload {
public:
    ShaderHotReload();
    ~ShaderHotReload();

    // Initialize with shader directory
    void Initialize(const fs::path& shaderDirectory);
    void Shutdown();

    // Update - call every frame to process file changes
    void Update();

    // Register a shader for hot-reload monitoring
    void RegisterShader(Ref<Shader> shader, const fs::path& filepath);
    void UnregisterShader(const fs::path& filepath);

    // Callbacks for reload events
    void OnShaderReloaded(ShaderReloadCallback callback);

    // Force reload a specific shader
    bool ReloadShader(const fs::path& filepath);

    // Force reload all registered shaders
    void ReloadAll();

    // Configuration
    void SetEnabled(bool enabled) { m_Enabled = enabled; }
    bool IsEnabled() const { return m_Enabled; }

    // Statistics
    struct Statistics {
        u32 RegisteredShaders = 0;
        u32 ReloadsThisSession = 0;
        u32 FailedReloads = 0;
        f32 LastReloadTimeMs = 0.0f;
    };

    const Statistics& GetStats() const { return m_Stats; }

    // Get the file watcher (for adding additional filters/callbacks)
    FileWatcher& GetFileWatcher() { return m_FileWatcher; }

private:
    void OnFileChanged(const FileEvent& event);

    // Resolve #include directives and get all dependencies
    Vector<fs::path> GetShaderDependencies(const fs::path& shaderPath);

    // Try to reload a shader, returns true on success
    bool TryReloadShader(const fs::path& filepath, Ref<Shader> shader);

private:
    FileWatcher m_FileWatcher;

    struct RegisteredShader {
        Ref<Shader> ShaderRef;
        fs::path FilePath;
        Vector<fs::path> Dependencies;  // #include files
    };

    // Map from filepath to registered shader
    HashMap<String, RegisteredShader> m_RegisteredShaders;

    // Map from dependency filepath to shaders that include it
    HashMap<String, Vector<String>> m_DependencyMap;

    Vector<ShaderReloadCallback> m_Callbacks;
    Statistics m_Stats;

    fs::path m_ShaderDirectory;
    bool m_Enabled = true;
    bool m_Initialized = false;
};

} // namespace Engine
