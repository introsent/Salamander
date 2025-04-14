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

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW ON
)

# Fetch GLM
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
    GIT_SHALLOW ON
)

# Fetch Vulkan Memory Allocator (VMA)
FetchContent_Declare(
    vma
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.2.1
    GIT_SHALLOW ON
)

# Fetch all dependencies
FetchContent_MakeAvailable(glfw glm vma)

# Define the path to your stb headers.
# Adjust the path below if your stb_image.h is in a different location.
set(STB_INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/external/stb" CACHE PATH "Path to stb headers")

# Set the project source files
set(${PROJECT_NAME}_SOURCES
    "src/main.cpp"
    "src/vma_impl.cpp" # Added implementation file for VMA
    "src/stb_image.h"
    "src/tiny_obj_loader.h"
    "src/core/window.h"
    "src/core/window.cpp" 
    "src/core/context.h" 
    "src/core/context.cpp" 
    "src/core/debug_messenger.h" 
    "src/core/debug_messenger.cpp" 
    "src/core/swap_chain.h"
    "src/core/swap_chain.cpp" "src/core/image_views.h" "src/core/image_views.cpp" "src/core/framebuffer_manager.h" "src/core/framebuffer_manager.cpp" "src/rendering/pipeline.h" "src/rendering/pipeline.cpp" "src/rendering/pipeline_config.h" "src/rendering/pipeline_config.cpp" "src/core/data_structures.h"  "src/resources/command_manager.h" "src/resources/command_manager.cpp" "src/resources/buffer_manager.h" "src/resources/buffer_manager.cpp" "src/resources/texture_manager.h" "src/resources/texture_manager.cpp" "src/rendering/render_pass.h" "src/rendering/render_pass.cpp" "src/rendering/descriptor_set_layout.h" "src/rendering/descriptor_set_layout.cpp" "src/rendering/descriptor_manager.h" "src/rendering/descriptor_manager.cpp")

# Add the project executable
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})

# Link libraries and include directories
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm)

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${vma_SOURCE_DIR}/include
    ${STB_INCLUDE_DIR}    # Add stb include directory
)

#
# Shader compilation via glslc
#

# Ensure VULKAN_SDK is set, as it provides glslc
if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "VULKAN_SDK environment variable is not set!")
endif()

# Define path to glslc
set(GLSLC "$ENV{VULKAN_SDK}/Bin/glslc.exe")

# Set the shader source directory and SPIR-V output directory
set(SHADERS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
set(SPIRV_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(MAKE_DIRECTORY ${SPIRV_DIR})

# Glob all shader source files (.vert and .frag)
file(GLOB SHADER_FILES "${SHADERS_SOURCE_DIR}/*.vert" "${SHADERS_SOURCE_DIR}/*.frag")

# List to hold compiled shader outputs
set(COMPILED_SHADERS)

# Loop over each shader and add a custom command for compilation
foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)  # e.g., ".vert" or ".frag"
    # Remove the leading '.' from the extension for the output file
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

# Create a custom target to compile all shaders
add_custom_target(CompileShaders ALL DEPENDS ${COMPILED_SHADERS})

# Ensure shaders are compiled before building the executable
add_dependencies(${PROJECT_NAME} CompileShaders)

# Copy additional SPIR-V resource files if needed (optional)
set(SHADERS_SPIRV_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders")
file(GLOB_RECURSE RESOURCE_FILES "${SHADERS_SPIRV_DIR}/*.spv")
foreach(RESOURCE ${RESOURCE_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${RESOURCE} ${SHADERS_SPIRV_DIR}
    )
endforeach()

# Ensure the textures directory exists in the build directory
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/textures)
file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/models)

# Copy the texture file from the source to the build directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/textures/viking_room.png 
               ${CMAKE_CURRENT_BINARY_DIR}/textures/viking_room.png 
               COPYONLY)

               
# Copy the model file
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/models/viking_room.obj
               ${CMAKE_CURRENT_BINARY_DIR}/models/viking_room.obj
               COPYONLY)
