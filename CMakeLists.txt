# Set minimum version of CMake
cmake_minimum_required(VERSION 3.27)

# Project Name
project(GP2_Vulkan)

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

# Set the output directory
set(${PROJECT_NAME}_SOURCES
    "src/main.cpp"
    "src/vma_impl.cpp"  # Added implementation file for VMA
)

# Add the project executable
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw glm)

# Include VMA headers
target_include_directories(${PROJECT_NAME} PRIVATE ${vma_SOURCE_DIR}/include)

# Copy resources to output folder
set(SHADERS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/shaders")
file(GLOB_RECURSE RESOURCE_FILES "${SHADERS_SOURCE_DIR}/*.spv")

set(RESOURCES_OUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/shaders/")
file(MAKE_DIRECTORY ${RESOURCES_OUT_DIR})

foreach(RESOURCE ${RESOURCE_FILES})
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${RESOURCE} ${RESOURCES_OUT_DIR}
    )
endforeach()
