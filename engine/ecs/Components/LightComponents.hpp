#pragma once

#include "ecs/Component.hpp"
#include "core/Types.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace Engine {

// ============================================================================
// Directional Light - Sun, moon, global ambient
// Direction is normalized, pointing TOWARDS the light source
// ============================================================================
struct DirectionalLightComponent {
    glm::vec3 Color{1.0f, 1.0f, 1.0f};
    f32 Intensity = 1.0f;
    glm::vec3 Direction{0.0f, -1.0f, 0.0f};
    bool CastShadows = true;
    bool Enabled = true;

    // Shadow configuration
    f32 ShadowBias = 0.0005f;
    f32 NormalBias = 0.02f;
    f32 ShadowSoftness = 1.0f;
    u32 ShadowResolution = 2048;
    f32 CascadeSplitLambda = 0.75f;  // 0=linear, 1=logarithmic
};

// ============================================================================
// Point Light - Bulbs, torches, etc.
// Uses inverse-square attenuation: 1 / (Constant + Linear*d + Quadratic*d^2)
// ============================================================================
struct PointLightComponent {
    glm::vec3 Color{1.0f, 1.0f, 1.0f};
    f32 Intensity = 1.0f;
    f32 Radius = 10.0f;
    f32 Constant = 1.0f;
    f32 Linear = 0.09f;
    f32 Quadratic = 0.032f;
    bool CastShadows = false;
    bool Enabled = true;
};

// ============================================================================
// Spot Light - Flashlights, stage lights, etc.
// Inner/Outer cutoff angles define the cone
// ============================================================================
struct SpotLightComponent {
    glm::vec3 Color{1.0f, 1.0f, 1.0f};
    f32 Intensity = 1.0f;
    glm::vec3 Direction{0.0f, -1.0f, 0.0f};
    f32 InnerCutOff = glm::radians(12.5f);
    f32 OuterCutOff = glm::radians(17.5f);
    f32 Range = 50.0f;
    f32 Constant = 1.0f;
    f32 Linear = 0.09f;
    f32 Quadratic = 0.032f;
    bool CastShadows = false;
    bool Enabled = true;
};

// ============================================================================
// Ambient Light - Global illumination fallback
// ============================================================================
struct AmbientLightComponent {
    glm::vec3 Color{0.03f, 0.03f, 0.03f};
    f32 Intensity = 1.0f;
};

} // namespace Engine

// ============================================================================
// Reflection Registrations
// ============================================================================
REFLECT_COMPONENT(Engine::DirectionalLightComponent,
    .data<&Engine::DirectionalLightComponent::Color>("Color"_hs)
    .data<&Engine::DirectionalLightComponent::Intensity>("Intensity"_hs)
    .data<&Engine::DirectionalLightComponent::Direction>("Direction"_hs)
    .data<&Engine::DirectionalLightComponent::CastShadows>("CastShadows"_hs)
    .data<&Engine::DirectionalLightComponent::Enabled>("Enabled"_hs)
    .data<&Engine::DirectionalLightComponent::ShadowBias>("ShadowBias"_hs)
    .data<&Engine::DirectionalLightComponent::NormalBias>("NormalBias"_hs)
    .data<&Engine::DirectionalLightComponent::ShadowSoftness>("ShadowSoftness"_hs)
    .data<&Engine::DirectionalLightComponent::ShadowResolution>("ShadowResolution"_hs)
    .data<&Engine::DirectionalLightComponent::CascadeSplitLambda>("CascadeSplitLambda"_hs)
);

REFLECT_COMPONENT(Engine::PointLightComponent,
    .data<&Engine::PointLightComponent::Color>("Color"_hs)
    .data<&Engine::PointLightComponent::Intensity>("Intensity"_hs)
    .data<&Engine::PointLightComponent::Radius>("Radius"_hs)
    .data<&Engine::PointLightComponent::Constant>("Constant"_hs)
    .data<&Engine::PointLightComponent::Linear>("Linear"_hs)
    .data<&Engine::PointLightComponent::Quadratic>("Quadratic"_hs)
    .data<&Engine::PointLightComponent::CastShadows>("CastShadows"_hs)
    .data<&Engine::PointLightComponent::Enabled>("Enabled"_hs)
);

REFLECT_COMPONENT(Engine::SpotLightComponent,
    .data<&Engine::SpotLightComponent::Color>("Color"_hs)
    .data<&Engine::SpotLightComponent::Intensity>("Intensity"_hs)
    .data<&Engine::SpotLightComponent::Direction>("Direction"_hs)
    .data<&Engine::SpotLightComponent::InnerCutOff>("InnerCutOff"_hs)
    .data<&Engine::SpotLightComponent::OuterCutOff>("OuterCutOff"_hs)
    .data<&Engine::SpotLightComponent::Range>("Range"_hs)
    .data<&Engine::SpotLightComponent::CastShadows>("CastShadows"_hs)
    .data<&Engine::SpotLightComponent::Enabled>("Enabled"_hs)
);

REFLECT_COMPONENT(Engine::AmbientLightComponent,
    .data<&Engine::AmbientLightComponent::Color>("Color"_hs)
    .data<&Engine::AmbientLightComponent::Intensity>("Intensity"_hs)
);
