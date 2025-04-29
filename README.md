
  <h1>Salamander Renderer</h1>
  <p>
    Salamander is a modern real-time 3D renderer built with Vulkan and C++ (targeting C++17). 
    It serves as a standalone graphics engine to demonstrate Vulkan’s capabilities, 
    with a focus on maintainability and proper design practices.
  </p>

  <h2>Features</h2>
  <ul>
    <li><strong>Vulkan-based Rendering</strong>: Constructs a Vulkan instance, selects a GPU, and creates a swapchain for real-time rendering.</li>
    <li><strong>Modular Engine</strong>: Clear separation of window management, Vulkan context, rendering pipeline, and resource loading.</li>
    <li><strong>Example Scene</strong>: Loads and renders a sample textured 3D scene ("viking room" model) with camera orbit and zoom controls.</li>
    <li><strong>Synchronization</strong>: Double-buffered rendering with semaphores and fences to synchronize CPU/GPU work.</li>
    <li><strong>Utilities</strong>: Texture, buffer, and descriptor set managers, along with a deletion queue for resource cleanup.</li>
    <li><strong>Extensible Pipeline</strong>: Customizable graphics pipeline for flexible shader and render pass management.</li>
  </ul>

  <h2>Dependencies</h2>
  <ul>
    <li><a href="https://vulkan.lunarg.com/">Vulkan SDK</a> (runtime, headers, validation layers)</li>
    <li><a href="https://www.glfw.org/">GLFW</a> (window/context management)</li>
    <li><a href="https://github.com/g-truc/glm">GLM</a> (mathematics library)</li>
    <li><a href="https://github.com/GPUOpen-LibrariesAndSDKs/VulkanMemoryAllocator">Vulkan Memory Allocator (VMA)</a></li>
    <li><a href="https://github.com/nothings/stb">stb_image</a> (image loading)</li>
    <li><a href="https://github.com/tinyobjloader/tinyobjloader">tinyobjloader</a> (OBJ mesh loading)</li>
  </ul>
  <p><em>Note: GLFW, GLM, stb_image, and tinyobjloader are already bundled in the source.</em></p>

  <h2>Build Instructions (Linux Only)</h2>

  <ol>
    <li><strong>Install Vulkan SDK and CMake (≥ 3.27)</strong>:
      <p>You can open the project directly in Visual Studio using CMake integration. Ensure the Vulkan SDK is installed and properly configured on your system.</p>
    </li>
  </ol>

  <hr>

  <p>
    For future development, Salamander aims to evolve into a feature-rich engine suitable 
    for research and learning purposes in real-time graphics and Vulkan API usage.
  </p>

