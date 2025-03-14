cmake_minimum_required(VERSION 3.14)
project(VKApp LANGUAGES CXX)

# Use FetchContent to retrieve dependencies
include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG latest
)
FetchContent_MakeAvailable(glfw)

# Fetch GLM (header-only)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG master
)
FetchContent_MakeAvailable(glm)

# Fetch Vulkan Memory Allocator (VMA)
FetchContent_Declare(
    VulkanMemoryAllocator
    GIT_REPOSITORY https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator.git
    GIT_TAG v3.2.1
)
FetchContent_MakeAvailable(VulkanMemoryAllocator)

# Find Vulkan SDK (assumes environment variable is set)
find_package(Vulkan REQUIRED)

# Setup shader directories and files
set(SHADER_DIR ${CMAKE_SOURCE_DIR}/shaders)
file(GLOB SHADER_FILES 
    "${SHADER_DIR}/*.vert"
    "${SHADER_DIR}/*.frag"
)

if(NOT DEFINED ENV{VULKAN_SDK})
    message(FATAL_ERROR "VULKAN_SDK environment variable is not set!")
endif()

set(GLSLC "$ENV{VULKAN_SDK}/Bin/glslc.exe")
set(SPIRV_DIR ${CMAKE_BINARY_DIR}/shaders)
file(MAKE_DIRECTORY ${SPIRV_DIR})  # Ensure output directory exists

# Compile each shader into SPIR-V
set(COMPILED_SHADERS)
foreach(SHADER ${SHADER_FILES})
    get_filename_component(SHADER_NAME ${SHADER} NAME_WE)
    get_filename_component(SHADER_EXT ${SHADER} EXT)  # e.g., ".vert"
    string(LENGTH ${SHADER_EXT} EXT_LENGTH)
    math(EXPR SUBSTRING_LENGTH "${EXT_LENGTH} - 1")
    string(SUBSTRING ${SHADER_EXT} 1 ${SUBSTRING_LENGTH} SHADER_TYPE)
    set(SPIRV_NAME ${SHADER_NAME}_${SHADER_TYPE}.spv)
    set(SPIRV_OUTPUT ${SPIRV_DIR}/${SPIRV_NAME})

    add_custom_command(
        OUTPUT ${SPIRV_OUTPUT}
        COMMAND ${GLSLC} ${SHADER} -o ${SPIRV_OUTPUT}
        DEPENDS ${SHADER}
        COMMENT "Compiling shader: ${SHADER} -> ${SPIRV_NAME}"
        VERBATIM
    )

    list(APPEND COMPILED_SHADERS ${SPIRV_OUTPUT})
endforeach()

# Create a target that builds all shaders
add_custom_target(CompileShaders ALL DEPENDS ${COMPILED_SHADERS})

# Create the main executable
add_executable(${PROJECT_NAME}
    src/main.cpp
    ${SHADER_FILES}
)

target_compile_features(${PROJECT_NAME} PRIVATE cxx_std_20)

# Link required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    Vulkan::Vulkan
    glm::glm
)

# Include directories for GLM and VMA (VMA is header-only)
target_include_directories(${PROJECT_NAME} PRIVATE
    ${glm_SOURCE_DIR}
    ${VulkanMemoryAllocator_SOURCE_DIR}/include
)

# Ensure shaders are compiled before building the executable
add_dependencies(${PROJECT_NAME} CompileShaders)

# Organize Visual Studio project folders
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
