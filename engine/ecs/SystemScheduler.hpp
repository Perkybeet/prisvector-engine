#pragma once

#include "System.hpp"
#include "core/Types.hpp"
#include "core/Logger.hpp"
#include <algorithm>
#include <chrono>
#include <array>

namespace Engine {

// Manages system execution order and lifecycle
class SystemScheduler {
public:
    SystemScheduler() = default;
    ~SystemScheduler() = default;

    // Add a system - returns pointer to the created system
    template<typename T, typename... Args>
    T* AddSystem(Args&&... args) {
        static_assert(std::is_base_of_v<ISystem, T>, "T must derive from ISystem");

        auto system = CreateScope<T>(std::forward<Args>(args)...);
        T* ptr = system.get();

        SystemPhase phase = ptr->GetPhase();
        auto& phaseList = m_Systems[static_cast<usize>(phase)];
        phaseList.push_back(std::move(system));

        SortPhase(phase);

        LOG_CORE_INFO("Added system '{}' to phase {} with priority {}",
                      ptr->GetName(),
                      SystemPhaseToString(phase),
                      ptr->GetPriority());

        return ptr;
    }

    // Remove a system by pointer
    void RemoveSystem(ISystem* system) {
        if (!system) return;

        SystemPhase phase = system->GetPhase();
        auto& phaseList = m_Systems[static_cast<usize>(phase)];

        auto it = std::find_if(phaseList.begin(), phaseList.end(),
            [system](const Scope<ISystem>& s) { return s.get() == system; });

        if (it != phaseList.end()) {
            LOG_CORE_INFO("Removed system '{}'", system->GetName());
            phaseList.erase(it);
        }
    }

    // Get a system by type
    template<typename T>
    T* GetSystem() {
        for (auto& phaseList : m_Systems) {
            for (auto& system : phaseList) {
                if (T* typed = dynamic_cast<T*>(system.get())) {
                    return typed;
                }
            }
        }
        return nullptr;
    }

    // Initialize all systems
    void Initialize(entt::registry& registry) {
        for (auto& phaseList : m_Systems) {
            for (auto& system : phaseList) {
                system->OnCreate(registry);
            }
        }
        m_Initialized = true;
    }

    // Shutdown all systems
    void Shutdown(entt::registry& registry) {
        // Shutdown in reverse order
        for (i32 phase = static_cast<i32>(SystemPhase::Count) - 1; phase >= 0; --phase) {
            auto& phaseList = m_Systems[phase];
            for (auto it = phaseList.rbegin(); it != phaseList.rend(); ++it) {
                (*it)->OnDestroy(registry);
            }
        }
        m_Initialized = false;
    }

    // Update all systems in order
    void Update(entt::registry& registry, f32 deltaTime) {
        for (u8 phase = 0; phase < static_cast<u8>(SystemPhase::Count); ++phase) {
            UpdatePhase(registry, static_cast<SystemPhase>(phase), deltaTime);
        }
    }

    // Update only a specific phase
    void UpdatePhase(entt::registry& registry, SystemPhase phase, f32 deltaTime) {
        auto& phaseList = m_Systems[static_cast<usize>(phase)];

        for (auto& system : phaseList) {
            if (!system->IsEnabled()) continue;

            auto start = std::chrono::high_resolution_clock::now();

            system->OnUpdate(registry, deltaTime);

            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration<f32, std::milli>(end - start).count();
            system->SetLastExecutionTime(duration);
        }
    }

    // Notify all systems of hot-reload event
    void NotifyReload() {
        for (auto& phaseList : m_Systems) {
            for (auto& system : phaseList) {
                system->OnReload();
            }
        }
    }

    // Get statistics
    struct Statistics {
        u32 TotalSystems = 0;
        u32 EnabledSystems = 0;
        f32 TotalExecutionTime = 0.0f;
        HashMap<String, f32> SystemExecutionTimes;
    };

    Statistics GetStatistics() const {
        Statistics stats;

        for (const auto& phaseList : m_Systems) {
            for (const auto& system : phaseList) {
                ++stats.TotalSystems;
                if (system->IsEnabled()) {
                    ++stats.EnabledSystems;
                }
                f32 execTime = system->GetLastExecutionTime();
                stats.TotalExecutionTime += execTime;
                stats.SystemExecutionTimes[system->GetName()] = execTime;
            }
        }

        return stats;
    }

    // Iterate over all systems
    template<typename Func>
    void ForEachSystem(Func&& func) {
        for (auto& phaseList : m_Systems) {
            for (auto& system : phaseList) {
                func(system.get());
            }
        }
    }

    // Iterate over systems in a specific phase
    template<typename Func>
    void ForEachSystemInPhase(SystemPhase phase, Func&& func) {
        auto& phaseList = m_Systems[static_cast<usize>(phase)];
        for (auto& system : phaseList) {
            func(system.get());
        }
    }

    bool IsInitialized() const { return m_Initialized; }

private:
    void SortPhase(SystemPhase phase) {
        auto& phaseList = m_Systems[static_cast<usize>(phase)];
        std::sort(phaseList.begin(), phaseList.end(),
            [](const Scope<ISystem>& a, const Scope<ISystem>& b) {
                return a->GetPriority() < b->GetPriority();
            });
    }

private:
    std::array<Vector<Scope<ISystem>>, static_cast<usize>(SystemPhase::Count)> m_Systems;
    bool m_Initialized = false;
};

} // namespace Engine
