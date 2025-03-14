cmake_minimum_required(VERSION 3.20)
project(VulkanApp)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include(FetchContent)

# Fetch GLFW
FetchContent_Declare(
    glfw
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG latest
)
FetchContent_MakeAvailable(glfw)

# Fetch GLM
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

# Find Vulkan
find_package(Vulkan REQUIRED)

# Define executable target
add_executable(VulkanApp src/main.cpp)

# Include directories
target_include_directories(VulkanApp PRIVATE 
    ${Vulkan_INCLUDE_DIRS} 
    ${VulkanMemoryAllocator_SOURCE_DIR}/include # Add VMA include path
)

# Link libraries
target_link_libraries(VulkanApp PRIVATE 
    Vulkan::Vulkan 
    glfw 
    glm::glm
)
