# Set minimum version of CMake
cmake_minimum_required(VERSION 3.27)

# Project Name
project(Salamander)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Check if Vulkan is installed
find_package(Vulkan REQUIRED)
if (NOT Vulkan_FOUND)
    message(FATAL_ERROR "Vulkan not found!")
endif()

# Include FetchContent to download external libraries
include(FetchContent)

# Fetch GLFW with proper build settings
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW ON
)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Fetch GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW ON
)
set(GLM_ENABLE_CXX_20 ON)

# Fetch Vulkan Memory Allocator (VMA)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.2.1
    GIT_SHALLOW ON
)

# Fetch all dependencies
FetchContent_MakeAvailable(glfw glm vma)


# Define the path to your stb headers
set(STB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/stb" CACHE PATH "Path to stb headers")

# Set the project source files
set(${PROJECT_NAME}_SOURCES
    "src/main.cpp"
    "src/includes/vma_impl.cpp"
    "src/includes/stb_image.h"
    "src/includes/tiny_obj_loader.h"
    "src/includes/tiny_gltf.h"
    "src/includes/stb_image_write.h"
    "src/includes/json.hpp"
    "src/core/window.h"
    "src/core/window.cpp" 
    "src/core/context.h" 
    "src/core/context.cpp" 
    "src/core/debug_messenger.h" 
    "src/core/debug_messenger.cpp" 
    "src/core/swap_chain.h"
    "src/core/swap_chain.cpp" 
    "src/core/image_views.h" 
    "src/core/image_views.cpp" 
    "src/core/framebuffer_manager.h" 
    "src/core/framebuffer_manager.cpp" 
    "src/rendering/pipeline.h" 
    "src/rendering/pipeline.cpp" 
    "src/rendering/pipeline_config.h" 
    "src/rendering/pipeline_config.cpp" 
    "src/core/data_structures.h"  
    "src/resources/command_manager.h" 
    "src/resources/command_manager.cpp" 
    "src/resources/buffer_manager.h" 
    "src/resources/buffer_manager.cpp" 
    "src/resources/texture_manager.h" 
    "src/resources/texture_manager.cpp" 
    "src/rendering/render_pass.h" 
    "src/rendering/render_pass.cpp" 
    "src/rendering/descriptors/descriptor_set_layout.h"
    "src/rendering/descriptors/descriptor_set_layout.cpp"
    "src/rendering/descriptors/descriptor_manager_base.h"
    "src/rendering/depth_format.h" 
    "src/rendering/depth_format.cpp" 
    "src/resources/command_buffer.h" 
    "src/resources/command_buffer.cpp" 
    "src/resources/command_pool_manager.h"
    "src/resources/command_pool_manager.cpp" 
    "src/resources/ibuffer.h" 
    "src/resources/buffer.h" 
    "src/resources/vertex_buffer.h" 
    "src/resources/vertex_buffer.cpp" 
    "src/resources/index_buffer.h" 
    "src/resources/index_buffer.cpp" 
    "src/resources/uniform_buffer.h" 
    "src/resources/uniform_buffer.cpp" 
    "src/renderer.h" 
    "src/renderer.cpp" 
    "src/application.h" 
    "src/application.cpp" 
    "src/deletion_queue.h"
    "src/executors/render_pass_executor.h"
    "src/user/user_executors/imgui_pass_executor.cpp"
    "src/user/user_executors/imgui_pass_executor.h"
    "src/rendering/descriptors/descriptor_set_layout_builder.cpp"
    "src/rendering/descriptors/descriptor_set_layout_builder.h"
    src/rendering/descriptors/descriptor_pool_builder.cpp
    src/rendering/descriptors/descriptor_pool_builder.h
    src/user/user_descriptor_managers/main_descriptor_manager.cpp
    src/user/user_descriptor_managers/main_descriptor_manager.h
    src/user/user_descriptor_managers/imgui_descriptor_manager.cpp
    src/user/user_descriptor_managers/imgui_descriptor_manager.h
    src/rendering/target/render_target.h
        src/user/user_render_targets/main_scene_target.cpp
        src/user/user_render_targets/main_scene_target.h
    src/user/user_render_targets/imgui_target.cpp
    src/user/user_render_targets/imgui_target.h
        src/rendering/image_transition_manager.h
    src/resources/ssbo_buffer.cpp
    src/resources/ssbo_buffer.h
    src/rendering/camera/camera.cpp
    src/rendering/camera/camera.h
        src/resources/loaders/gltf_loader.cpp
        src/resources/loaders/gltf_loader.h
        src/user/user_passes/depth_prepass.cpp
        src/user/user_passes/depth_prepass.h
        src/user/user_passes/gbuffer_pass.cpp
        src/user/user_passes/gbuffer_pass.h
        src/user/user_passes/lighting_pass.cpp
        src/user/user_passes/lighting_pass.h
        src/user/user_passes/tone_mapping_pass.cpp
        src/user/user_passes/tone_mapping_pass.h
        src/user/user_render_targets/main_scene_controller.cpp
        src/user/user_render_targets/main_scene_controller.h
        src/user/user_render_targets/cube_map_renderer.cpp
        src/user/user_render_targets/cube_map_renderer.h
)

# Add ImGui source files with proper includes and links
add_library(ImGui STATIC
    "src/imgui/imgui.cpp"
    "src/imgui/imgui_draw.cpp"
    "src/imgui/imgui_tables.cpp"
    "src/imgui/imgui_widgets.cpp"
    "src/imgui/imgui_demo.cpp"
    "src/imgui/backends/imgui_impl_vulkan.cpp"
    "src/imgui/backends/imgui_impl_glfw.cpp"
)

# ImGui include directories with GLFW and Vulkan includes
target_include_directories(ImGui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/backends
    ${glfw_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
)

# ImGui needs to link against GLFW and Vulkan
target_link_libraries(ImGui PRIVATE
    glfw
    Vulkan::Vulkan
)

# Add the project executable
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})

# Link libraries and include directories
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ImGui 
    Vulkan::Vulkan 
    glfw 
    glm
)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${vma_SOURCE_DIR}/include
    ${STB_INCLUDE_DIR}
    ${glfw_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/src/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/rendering
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/user
    ${CMAKE_CURRENT_SOURCE_DIR}/src/executors
    ${CMAKE_CURRENT_SOURCE_DIR}/src/includes
    ${CMAKE_CURRENT_BINARY_DIR}/include
)

# Shader compilation via glslc
if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "VULKAN_SDK environment variable is not set!")
endif()

set(GLSLC "$ENV{VULKAN_SDK}/Bin/glslc.exe")
set(SHADERS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SPIRV_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SPIRV_DIR})

file(GLOB SHADER_FILES "${SHADERS_SOURCE_DIR}/*.vert" "${SHADERS_SOURCE_DIR}/*.frag")
set(COMPILED_SHADERS)

foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)
    string(LENGTH ${SHADER_EXT} EXT_LENGTH)
    math(EXPR SUBSTRING_LENGTH "${EXT_LENGTH} - 1")
    string(SUBSTRING ${SHADER_EXT} 1 ${SUBSTRING_LENGTH} SHADER_TYPE)
    set(SPIRV_OUTPUT "${SPIRV_DIR}/${SHADER_NAME}_${SHADER_TYPE}.spv")

    add_custom_command(
        OUTPUT ${SPIRV_OUTPUT}
        COMMAND ${GLSLC} ${SHADER} -o ${SPIRV_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${SHADER_NAME}${SHADER_EXT} -> ${SHADER_NAME}_${SHADER_TYPE}.spv"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${SPIRV_OUTPUT})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${COMPILED_SHADERS})
add_dependencies(${PROJECT_NAME} CompileShaders)

# Resource copying
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/textures)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
        @ONLY
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/textures/viking_room.png 
               ${CMAKE_CURRENT_BINARY_DIR}/textures/viking_room.png
               ${CMAKE_CURRENT_SOURCE_DIR}/textures/circus_arena.hdr
               ${CMAKE_CURRENT_BINARY_DIR}/textures/circus_arena.hdr
                COPYONLY)
               
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/models/viking_room.obj
               ${CMAKE_CURRENT_BINARY_DIR}/models/viking_room.obj
               ${CMAKE_CURRENT_BINARY_DIR}/models/sponza/Sponza.gltf
               COPYONLY)

file(GLOB JPG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/models/sponza/*.jpg")
foreach(jpg_file ${JPG_FILES})
    get_filename_component(filename ${jpg_file} NAME)
    configure_file(${jpg_file}
            ${CMAKE_CURRENT_BINARY_DIR}/models/sponza/${filename}
            COPYONLY)
endforeach()