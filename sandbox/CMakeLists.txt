# Sandbox application
add_executable(Sandbox main.cpp)

# Find X11 for ImGui multi-viewport support on Linux
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
    target_link_libraries(Sandbox PRIVATE GameEngine ${X11_LIBRARIES})
else()
    target_link_libraries(Sandbox PRIVATE GameEngine)
endif()

# Copy assets to build directory
add_custom_command(TARGET Sandbox POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:Sandbox>/assets
)
