#pragma once

#include "ecs/Component.hpp"
#include "core/Types.hpp"
#include <entt/entt.hpp>

namespace Engine {

// Hierarchy component for parent-child relationships
// Uses linked list for siblings (efficient insertion/removal)
struct Hierarchy {
    entt::entity Parent = entt::null;
    entt::entity FirstChild = entt::null;
    entt::entity NextSibling = entt::null;
    entt::entity PrevSibling = entt::null;

    u32 ChildCount = 0;
    u32 Depth = 0;  // Depth in hierarchy tree (root = 0)

    // Check if this entity has a parent
    bool HasParent() const {
        return Parent != entt::null;
    }

    // Check if this entity has children
    bool HasChildren() const {
        return FirstChild != entt::null;
    }

    // Check if this entity is root (no parent)
    bool IsRoot() const {
        return Parent == entt::null;
    }
};

// Tag component for root entities (optimization for queries)
struct RootEntity {};

// Helper functions for hierarchy management
namespace HierarchyUtils {

    // Forward declaration
    inline void UpdateChildDepths(entt::registry& registry, entt::entity entity, u32 parentDepth);

    // Set parent of entity (handles all linking)
    inline void SetParent(entt::registry& registry,
                         entt::entity child,
                         entt::entity newParent) {
        auto& childHierarchy = registry.get_or_emplace<Hierarchy>(child);

        // Remove from old parent if any
        if (childHierarchy.Parent != entt::null) {
            auto* oldParentHierarchy = registry.try_get<Hierarchy>(childHierarchy.Parent);
            if (oldParentHierarchy) {
                // Unlink from sibling chain
                if (childHierarchy.PrevSibling != entt::null) {
                    auto& prev = registry.get<Hierarchy>(childHierarchy.PrevSibling);
                    prev.NextSibling = childHierarchy.NextSibling;
                } else {
                    // Was first child
                    oldParentHierarchy->FirstChild = childHierarchy.NextSibling;
                }

                if (childHierarchy.NextSibling != entt::null) {
                    auto& next = registry.get<Hierarchy>(childHierarchy.NextSibling);
                    next.PrevSibling = childHierarchy.PrevSibling;
                }

                oldParentHierarchy->ChildCount--;
            }

            // Remove root tag if had one
            registry.remove<RootEntity>(child);
        }

        // Set new parent
        childHierarchy.Parent = newParent;
        childHierarchy.NextSibling = entt::null;
        childHierarchy.PrevSibling = entt::null;

        if (newParent != entt::null) {
            auto& parentHierarchy = registry.get_or_emplace<Hierarchy>(newParent);

            // Add to front of children list
            if (parentHierarchy.FirstChild != entt::null) {
                auto& firstChild = registry.get<Hierarchy>(parentHierarchy.FirstChild);
                firstChild.PrevSibling = child;
                childHierarchy.NextSibling = parentHierarchy.FirstChild;
            }

            parentHierarchy.FirstChild = child;
            parentHierarchy.ChildCount++;

            // Update depth
            childHierarchy.Depth = parentHierarchy.Depth + 1;
        } else {
            // No parent = root entity
            registry.emplace_or_replace<RootEntity>(child);
            childHierarchy.Depth = 0;
        }

        // Recursively update children depths
        UpdateChildDepths(registry, child, childHierarchy.Depth);
    }

    // Update depths of all descendants
    inline void UpdateChildDepths(entt::registry& registry,
                                  entt::entity entity,
                                  u32 parentDepth) {
        auto* hierarchy = registry.try_get<Hierarchy>(entity);
        if (!hierarchy) return;

        entt::entity child = hierarchy->FirstChild;
        while (child != entt::null) {
            auto& childHierarchy = registry.get<Hierarchy>(child);
            childHierarchy.Depth = parentDepth + 1;
            UpdateChildDepths(registry, child, childHierarchy.Depth);
            child = childHierarchy.NextSibling;
        }
    }

    // Detach entity from parent (make it root)
    inline void DetachFromParent(entt::registry& registry, entt::entity entity) {
        SetParent(registry, entity, entt::null);
    }

    // Destroy entity and all its descendants
    inline void DestroyHierarchy(entt::registry& registry, entt::entity entity) {
        auto* hierarchy = registry.try_get<Hierarchy>(entity);
        if (hierarchy) {
            // Recursively destroy children
            entt::entity child = hierarchy->FirstChild;
            while (child != entt::null) {
                auto& childHierarchy = registry.get<Hierarchy>(child);
                entt::entity nextChild = childHierarchy.NextSibling;
                DestroyHierarchy(registry, child);
                child = nextChild;
            }

            // Detach from parent
            DetachFromParent(registry, entity);
        }

        // Destroy this entity
        registry.destroy(entity);
    }

    // Get root ancestor of entity
    inline entt::entity GetRoot(entt::registry& registry, entt::entity entity) {
        auto* hierarchy = registry.try_get<Hierarchy>(entity);
        while (hierarchy && hierarchy->Parent != entt::null) {
            entity = hierarchy->Parent;
            hierarchy = registry.try_get<Hierarchy>(entity);
        }
        return entity;
    }

    // Check if ancestor is an ancestor of entity
    inline bool IsAncestorOf(entt::registry& registry,
                             entt::entity ancestor,
                             entt::entity entity) {
        auto* hierarchy = registry.try_get<Hierarchy>(entity);
        while (hierarchy && hierarchy->Parent != entt::null) {
            if (hierarchy->Parent == ancestor) return true;
            entity = hierarchy->Parent;
            hierarchy = registry.try_get<Hierarchy>(entity);
        }
        return false;
    }

    // Iterate over all children (non-recursive)
    template<typename Func>
    void ForEachChild(entt::registry& registry, entt::entity parent, Func&& func) {
        auto* hierarchy = registry.try_get<Hierarchy>(parent);
        if (!hierarchy) return;

        entt::entity child = hierarchy->FirstChild;
        while (child != entt::null) {
            auto& childHierarchy = registry.get<Hierarchy>(child);
            entt::entity nextChild = childHierarchy.NextSibling;
            func(child);
            child = nextChild;
        }
    }

    // Iterate over all descendants (recursive, depth-first)
    template<typename Func>
    void ForEachDescendant(entt::registry& registry, entt::entity parent, Func&& func) {
        auto* hierarchy = registry.try_get<Hierarchy>(parent);
        if (!hierarchy) return;

        entt::entity child = hierarchy->FirstChild;
        while (child != entt::null) {
            auto& childHierarchy = registry.get<Hierarchy>(child);
            entt::entity nextChild = childHierarchy.NextSibling;
            func(child);
            ForEachDescendant(registry, child, func);
            child = nextChild;
        }
    }

} // namespace HierarchyUtils

} // namespace Engine

// Reflection registration
REFLECT_COMPONENT(Engine::Hierarchy,
    .data<&Engine::Hierarchy::Parent>("Parent"_hs)
    .data<&Engine::Hierarchy::ChildCount>("ChildCount"_hs)
    .data<&Engine::Hierarchy::Depth>("Depth"_hs)
);

REFLECT_TAG(Engine::RootEntity);
