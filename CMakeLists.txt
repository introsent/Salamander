# Set minimum version of CMake
cmake_minimum_required(VERSION 3.27)

# Project Name
project(Salamander)

# Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# USER SETTING: Loader selection options
option(USE_ASSIMP "Build with Assimp loader" ON)
option(USE_TINYGLTF "Build with tinygltf loader (header-only)" OFF)

if(USE_ASSIMP AND USE_TINYGLTF)
    message(FATAL_ERROR "Only one loader may be enabled: set either -DUSE_ASSIMP=ON or -DUSE_TINYGLTF=ON")
endif()
message(STATUS "Model loader selection:")
if(USE_ASSIMP)
    message(STATUS "  Using Assimp")
elseif(USE_TINYGLTF)
    message(STATUS "  Using tinygltf")
else()
    message(STATUS "  No explicit loader chosen. Defaulting to assimp")
    set(USE_ASSIMP ON)
endif()


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

if(USE_ASSIMP)
    # Fetch ASSIMP
    FetchContent_Declare(
            assimp
            GIT_REPOSITORY https://github.com/assimp/assimp.git
            GIT_TAG master
            GIT_SHALLOW ON
    )
    set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
    set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(assimp)

endif()

# Fetch Vulkan Memory Allocator (VMA)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.2.1
    GIT_SHALLOW ON
)

# Fetch all dependencies
FetchContent_MakeAvailable(glfw glm vma)

FetchContent_Declare(
        imgui_node_editor
        GIT_REPOSITORY https://github.com/thedmd/imgui-node-editor.git
        GIT_TAG master
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(imgui_node_editor)

add_library(ImGuiNodeEditor STATIC
        "${imgui_node_editor_SOURCE_DIR}/crude_json.cpp"
        "${imgui_node_editor_SOURCE_DIR}/imgui_canvas.cpp"
        "${imgui_node_editor_SOURCE_DIR}/imgui_node_editor.cpp"
        "${imgui_node_editor_SOURCE_DIR}/imgui_node_editor_api.cpp"
)
target_include_directories(ImGuiNodeEditor PUBLIC ${imgui_node_editor_SOURCE_DIR})
target_link_libraries(ImGuiNodeEditor PUBLIC ImGui)


# Define the path to your stb headers
set(STB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/stb" CACHE PATH "Path to stb headers")


# Set the project source files
set(${PROJECT_NAME}_SOURCES
    "src/main.cpp"
    "external/vma_impl.cpp"
    "external/stb_image.h"
    "external/tiny_obj_loader.h"
    "external/stb_image_write.h"
    "external/json.hpp"
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

    # graphics module (formerly rendering/)
    "src/graphics/pipeline/pipeline.h"
    "src/graphics/pipeline/pipeline.cpp"
    "src/graphics/pipeline/compute_pipeline.h"
    "src/graphics/pipeline/compute_pipeline.cpp"
    "src/graphics/pipeline/push_constants.h"
    "src/graphics/render_pass.h"
    "src/graphics/render_pass.cpp"
    "src/graphics/depth_format.h"
    "src/graphics/depth_format.cpp"
    "src/graphics/image_transition_manager.h"
    "src/graphics/descriptors/descriptor_set_layout.h"
    "src/graphics/descriptors/descriptor_set_layout.cpp"
    "src/graphics/descriptors/descriptor_manager_base.h"
    "src/graphics/descriptors/descriptor_set_layout_builder.h"
    "src/graphics/descriptors/descriptor_set_layout_builder.cpp"
    "src/graphics/descriptors/descriptor_pool_builder.h"
    "src/graphics/descriptors/descriptor_pool_builder.cpp"
    "src/graphics/descriptors/managers/main_descriptor_manager.h"
    "src/graphics/descriptors/managers/main_descriptor_manager.cpp"
    "src/graphics/descriptors/managers/imgui_descriptor_manager.h"
    "src/graphics/descriptors/managers/imgui_descriptor_manager.cpp"

    # resources module (now organized into subdirectories)
    "src/resources/buffers/command_manager.h"
    "src/resources/buffers/command_manager.cpp"
    "src/resources/buffers/buffer_manager.h"
    "src/resources/buffers/buffer_manager.cpp"
    "src/resources/buffers/command_buffer.h"
    "src/resources/buffers/command_buffer.cpp"
    "src/resources/buffers/command_pool_manager.h"
    "src/resources/buffers/command_pool_manager.cpp"
    "src/resources/buffers/ibuffer.h"
    "src/resources/buffers/buffer.h"
    "src/resources/buffers/vertex_buffer.h"
    "src/resources/buffers/vertex_buffer.cpp"
    "src/resources/buffers/index_buffer.h"
    "src/resources/buffers/index_buffer.cpp"
    "src/resources/buffers/uniform_buffer.h"
    "src/resources/buffers/uniform_buffer.cpp"
    "src/resources/buffers/ssbo_buffer.h"
    "src/resources/buffers/ssbo_buffer.cpp"
    "src/resources/textures/texture_manager.h"
    "src/resources/textures/texture_manager.cpp"
    "src/resources/textures/texture.h"
    "src/resources/textures/texture.cpp"
    "src/resources/textures/image.h"
    "src/resources/textures/image.cpp" 
    "src/render.h"
    "src/render.cpp"
    "src/application.h" 
    "src/application.cpp"

    # scene module (camera, components, lighting)
    "src/scene/camera/camera.h"
    "src/scene/camera/camera.cpp"
    "src/scene/camera/camera_exposure.h"
    "src/scene/components/vertex.h"
    "src/scene/components/material.h"
    "src/scene/components/transform.h"
    "src/scene/lighting/lights.h"
    "src/scene/scene_data.h"

    # renderer module (passes, targets, frame management)
    "src/renderer/frame/frame_data.h"
    "src/renderer/frame/frame_context.h"
    "src/renderer/frame/render_context.h"
    "src/renderer/frame/render_context.cpp"
    "src/renderer/passes/pass_dependencies.h"
    "src/renderer/passes/depth_prepass.h"
    "src/renderer/passes/depth_prepass.cpp"
    "src/renderer/passes/gbuffer_pass.h"
    "src/renderer/passes/gbuffer_pass.cpp"
    "src/renderer/passes/lighting_pass.h"
    "src/renderer/passes/lighting_pass.cpp"
    "src/renderer/passes/tone_mapping_pass.h"
    "src/renderer/passes/tone_mapping_pass.cpp"
    "src/renderer/passes/shadow_pass.h"
    "src/renderer/passes/shadow_pass.cpp"
    "src/renderer/passes/luminance_histogram_pass.h"
    "src/renderer/passes/luminance_histogram_pass.cpp"
    "src/renderer/passes/luminance_average_pass.h"
    "src/renderer/passes/luminance_average_pass.cpp"
    "src/renderer/passes/irender_pass.h"
    "src/renderer/targets/target/render_target.h"
    "src/renderer/targets/main_scene_target.h"
    "src/renderer/targets/main_scene_target.cpp"
    "src/renderer/targets/imgui_target.h"
    "src/renderer/targets/imgui_target.cpp"
    "src/renderer/targets/main_scene_controller.h"
    "src/renderer/targets/main_scene_controller.cpp"
    "src/renderer/targets/cube_map_renderer.h"
    "src/renderer/targets/cube_map_renderer.cpp"

    # executors
    "src/executors/render_pass_executor.h"
    "src/executors/imgui_pass_executor.h"
    "src/executors/imgui_pass_executor.cpp"

    # utilities
    "src/deletion_queue.h"
        src/renderer/passes/depth_prepass.cpp
        src/renderer/render_graph/resource_description_types.h
        src/renderer/render_graph/render_resource_handles.h
        src/renderer/render_graph/graph.cpp
        src/renderer/render_graph/graph.h
        src/renderer/render_graph/resource_access.h
        src/renderer/render_graph/pass_builder.cpp
        src/renderer/render_graph/pass_builder.h
        src/renderer/render_graph/internal/pass_node.h
        src/renderer/render_graph/internal/resource_nodes.h
        src/helpers.cpp
        src/helpers.h
        src/renderer/render_graph/internal/barrier_descriptor.h
        src/renderer/debug/render_graph_debug_panel.cpp
        src/renderer/debug/render_graph_debug_panel.h
        src/renderer/debug/debug_panel.h
        src/renderer/debug/debug_panel_registry.h
        src/renderer/debug/debug_panel_registry.cpp
        src/renderer/debug/scene_debug_panel.h
)


if (USE_TINYGLTF)
    list(APPEND ${PROJECT_NAME}_SOURCES

            "src/resources/loaders/tinygltf_loader.cpp"
            "src/resources/loaders/tinygltf_loader.h"
    )
elseif (USE_ASSIMP)
    list(APPEND ${PROJECT_NAME}_SOURCES
            "src/resources/loaders/assimp_loader.cpp"
            "src/resources/loaders/assimp_loader.h"
    )
endif()

# Add ImGui source files with proper includes and links
add_library(ImGui STATIC
    "external/imgui/imgui.cpp"
    "external/imgui/imgui_draw.cpp"
    "external/imgui/imgui_tables.cpp"
    "external/imgui/imgui_widgets.cpp"
    "external/imgui/imgui_demo.cpp"
    "external/imgui/backends/imgui_impl_vulkan.cpp"
    "external/imgui/backends/imgui_impl_glfw.cpp"
)

# ImGui include directories with GLFW and Vulkan includes
target_include_directories(ImGui PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
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

if (USE_TINYGLTF)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_TINYGLTF)
elseif (USE_ASSIMP)
    target_compile_definitions(${PROJECT_NAME} PRIVATE USE_ASSIMP)
endif()

# Link libraries and include directories
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ImGui
    ImGuiNodeEditor
    Vulkan::Vulkan 
    glfw 
    glm
)
if (USE_ASSIMP)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        assimp::assimp
    )
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
    ${vma_SOURCE_DIR}/include
    ${STB_INCLUDE_DIR}
    ${glfw_SOURCE_DIR}/include
    ${assimp_SOURCE_DIR}/include
    ${Vulkan_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/external
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui
    ${CMAKE_CURRENT_SOURCE_DIR}/external/imgui/backends
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/executors
    ${CMAKE_CURRENT_SOURCE_DIR}/src/shared
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

file(GLOB SHADER_FILES "${SHADERS_SOURCE_DIR}/*.vert" "${SHADERS_SOURCE_DIR}/*.frag" "${SHADERS_SOURCE_DIR}/*.comp")
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
        COMMAND ${GLSLC} ${SHADER} -o ${SPIRV_OUTPUT} -g
        DEPENDS ${SHADER}
        COMMENT "Compiling shader ${SHADER_NAME}${SHADER_EXT} -> ${SHADER_NAME}_${SHADER_TYPE}.spv"
        VERBATIM
    )
    list(APPEND COMPILED_SHADERS ${SPIRV_OUTPUT})
endforeach()

add_custom_target(CompileShaders ALL DEPENDS ${COMPILED_SHADERS})
add_dependencies(${PROJECT_NAME} CompileShaders)

add_compile_definitions(
        GLM_FORCE_DEPTH_ZERO_TO_ONE
        GLM_FORCE_RADIANS
        GLM_FORCE_ALIGNED_GENTYPES
        GLM_ENABLE_EXPERIMENTAL
)

# Resource copying
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/textures)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/shaders)

configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in"
        "${CMAKE_CURRENT_BINARY_DIR}/include/config.h"
        @ONLY
)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/textures/circus_arena.hdr
               ${CMAKE_CURRENT_BINARY_DIR}/textures/circus_arena.hdr
                ${CMAKE_CURRENT_SOURCE_DIR}/textures/bambanani_sunset.hdr
                ${CMAKE_CURRENT_BINARY_DIR}/textures/bambanani_sunset.hdr
                COPYONLY)
               
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/models/sponza/Sponza.gltf
               COPYONLY)

file(GLOB JPG_FILES "${CMAKE_CURRENT_SOURCE_DIR}/models/sponza/*.jpg")
foreach(jpg_file ${JPG_FILES})
    get_filename_component(filename ${jpg_file} NAME)
    configure_file(${jpg_file}
            ${CMAKE_CURRENT_BINARY_DIR}/models/sponza/${filename}
            COPYONLY)
endforeach()