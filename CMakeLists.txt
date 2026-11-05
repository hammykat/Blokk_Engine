cmake_minimum_required(VERSION 3.25)

project(Blokk
    VERSION 0.1.9
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Disable Perl requiring AVIF support
set(SDLIMAGE_AVIF OFF CACHE BOOL "Disable AVIF support" FORCE)

# SDL
add_subdirectory(vendored/SDL)

# SDL_image
add_subdirectory(vendored/SDL_image)

# Vulkan
# Find the official Vulkan SDK on the system
find_package(Vulkan REQUIRED)

# Add the vk-bootstrap submodule directory
add_subdirectory(vendored/vk-bootstrap)

# GLM Math Library
add_subdirectory(vendored/glm)

# GLFW - Turn off unwanted targets to keep compilation fast
set(GLFW_BUILD_DOCS OFF CACHE INTERNAL "")
set(GLFW_BUILD_TESTS OFF CACHE INTERNAL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE INTERNAL "")
add_subdirectory(vendored/glfw)


# Imgui
add_library(imgui STATIC
    vendored/imgui/imgui.cpp
    vendored/imgui/imgui_draw.cpp
    vendored/imgui/imgui_tables.cpp
    vendored/imgui/imgui_widgets.cpp

    vendored/imgui/backends/imgui_impl_sdl3.cpp # For SDL3
    vendored/imgui/backends/imgui_impl_vulkan.cpp # Needed to use Vulkan
)

target_include_directories(imgui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendored/imgui
        ${CMAKE_CURRENT_SOURCE_DIR}/vendored/imgui/backends
)

target_link_libraries(imgui
    PUBLIC
        SDL3::SDL3
        Vulkan::Vulkan # ImGui Vulkan backend needs access to Vulkan headers
)

# xsimd
add_subdirectory(vendored/xsimd EXCLUDE_FROM_ALL)


# BFCL
add_subdirectory(vendored/BFCL)

# Blokk engine
add_subdirectory(Blokk/Engine/Systems)
