#pragma once

// Core
#include "core/Types.hpp"
#include "core/Logger.hpp"
#include "core/Application.hpp"
#include "core/Window.hpp"
#include "core/Input.hpp"
#include "core/InputBindings.hpp"
#include "core/Time.hpp"

// Events
#include "events/Event.hpp"
#include "events/WindowEvents.hpp"
#include "events/KeyEvents.hpp"
#include "events/MouseEvents.hpp"

// ECS (Entity Component System)
#include "ecs/Core.hpp"

// Renderer
#include "renderer/opengl/GLBuffer.hpp"
#include "renderer/opengl/GLVertexArray.hpp"
#include "renderer/opengl/GLShader.hpp"
#include "renderer/RenderCommand.hpp"
#include "renderer/RenderQueue.hpp"
#include "renderer/BatchRenderer.hpp"
#include "renderer/Texture.hpp"
#include "renderer/Mesh.hpp"
#include "renderer/pipeline/Framebuffer.hpp"
#include "renderer/pipeline/GBuffer.hpp"
#include "renderer/lighting/DeferredLightingSystem.hpp"
#include "renderer/shadows/ShadowMapSystem.hpp"

// Light Components
#include "ecs/Components/LightComponents.hpp"

// Resources
#include "resources/ResourceManager.hpp"
#include "resources/loaders/MeshLoader.hpp"

// Math
#include "math/Interpolation.hpp"
#include "math/AABB.hpp"
#include "math/Frustum.hpp"

// Camera
#include "camera/Camera.hpp"
#include "camera/PerspectiveCamera.hpp"
#include "camera/CameraController.hpp"
#include "camera/FPSCameraController.hpp"
#include "camera/ThirdPersonCameraController.hpp"
#include "camera/OrbitalCameraController.hpp"
#include "camera/CameraManager.hpp"

// Particles
#include "renderer/particles/ParticleTypes.hpp"
#include "renderer/particles/ParticleEmitter.hpp"
#include "renderer/particles/ParticleSystem.hpp"
