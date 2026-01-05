# Collect source files
file(GLOB_RECURSE ENGINE_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp"
)
file(GLOB_RECURSE ENGINE_HEADERS
    "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp"
)

# Create engine library
add_library(GameEngine STATIC ${ENGINE_SOURCES} ${ENGINE_HEADERS})

# Include directories
target_include_directories(GameEngine
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link dependencies
target_link_libraries(GameEngine
    PUBLIC
        glfw
        glad_gl_core_45
        glm::glm
        EnTT::EnTT
        imgui
        imguizmo
        spdlog::spdlog
        stb
)

# Compiler definitions
target_compile_definitions(GameEngine
    PUBLIC
        $<$<CONFIG:Debug>:ENGINE_DEBUG>
        $<$<CONFIG:Release>:ENGINE_RELEASE>
        GLFW_INCLUDE_NONE
        GLM_FORCE_RADIANS
        GLM_FORCE_DEPTH_ZERO_TO_ONE
)

# Enable warnings
if(MSVC)
    target_compile_options(GameEngine PRIVATE /W4)
else()
    target_compile_options(GameEngine PRIVATE -Wall -Wextra -Wpedantic)
endif()
