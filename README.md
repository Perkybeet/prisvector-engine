# Game Engine

A modern C++20 game engine built with OpenGL 4.5, featuring a complete editor interface and entity component system.

## Features

### Core Engine
- **Application Framework** - Window management, input handling, event system
- **Entity Component System** - Built on EnTT with 9-phase system execution
- **Resource Management** - Async loading, caching, and hot-reload support

### Rendering
- **Deferred Rendering Pipeline** - G-Buffer based lighting with PBR materials
- **Shadow Mapping** - Cascaded shadow maps (CSM) for directional lights, atlas-based spot light shadows
- **Particle System** - GPU compute-based particles with configurable emitters
- **Post-Processing** - HDR tonemapping, gamma correction

### Editor
- **Dockable Panels** - Scene hierarchy, inspector, viewport, console, asset browser
- **Transform Gizmos** - Translate, rotate, scale with snapping support
- **Editor Camera** - Orbit, pan, zoom controls
- **Play Mode** - Scene state preservation and restoration

### Camera System
- Multiple controller types: FPS, Third-Person, Orbital
- Smooth interpolation and configurable parameters

## Requirements

- C++20 compatible compiler (GCC 11+, Clang 13+, MSVC 2022+)
- CMake 3.20+
- OpenGL 4.5 capable GPU

## Building

```bash
# Clone the repository
git clone https://github.com/Perkybeet/prisvector-engine.git
cd prisvector-engine

# Configure and build (Release)
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

# Run the editor
./build/bin/Sandbox
```

### Build Options

| Option | Default | Description |
|--------|---------|-------------|
| `ENGINE_BUILD_SANDBOX` | ON | Build the demo application |
| `ENGINE_BUILD_TESTS` | OFF | Build unit tests |
| `ENGINE_ENABLE_PROFILING` | OFF | Enable performance profiling |

## Project Structure

```
engine/
├── core/           # Application, window, input, logging
├── ecs/            # Entity component system
├── renderer/       # OpenGL rendering, shaders, materials
├── camera/         # Camera controllers
├── editor/         # Editor panels and tools
├── events/         # Event dispatcher
├── math/           # AABB, Ray, interpolation utilities
├── resources/      # Asset loading and management
└── hotreload/      # File watching, shader hot-reload

assets/
└── shaders/        # GLSL shaders (deferred, shadows, particles, post-process)

sandbox/
└── demos/          # Demo applications
```

## Dependencies

All dependencies are fetched automatically via CMake FetchContent:

- [GLFW](https://www.glfw.org/) 3.4 - Window and input
- [GLAD](https://glad.dav1d.de/) 2.0.6 - OpenGL loader
- [GLM](https://github.com/g-truc/glm) 1.0.1 - Mathematics
- [EnTT](https://github.com/skypjack/entt) 3.13.2 - ECS framework
- [spdlog](https://github.com/gabime/spdlog) 1.14.1 - Logging
- [Dear ImGui](https://github.com/ocornut/imgui) 1.91.6 - Editor UI
- [ImGuizmo](https://github.com/CedricGuillemet/ImGuizmo) - Transform gizmos
- [stb](https://github.com/nothings/stb) - Image loading

## Controls

### Editor Camera
- `Alt + Left Mouse` - Orbit around focal point
- `Alt + Middle Mouse` - Pan
- `Alt + Right Mouse` - Zoom
- `Scroll Wheel` - Zoom

### Gizmos
- `W` - Translate mode
- `E` - Rotate mode
- `R` - Scale mode
- `Click` - Select entity

## License

This project is proprietary software. See [LICENSE](LICENSE) for terms.

## Author

Yago
