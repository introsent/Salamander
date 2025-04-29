<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <title>Salamander Renderer</title>
</head>
<body>

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
      <p>Use your system's package manager or download from the official sources.</p>
    </li>

    <li><strong>Clone the Repository</strong>:
      <pre><code>git clone &lt;repo_url&gt;
cd salamander</code></pre>
    </li>

    <li><strong>Set Project Root (Optional)</strong>:
      <pre><code>export SALAMANDER_ROOT=$(pwd)</code></pre>
    </li>

    <li><strong>Create a Build Directory</strong>:
      <pre><code>mkdir build &amp;&amp; cd build</code></pre>
    </li>

    <li><strong>Run CMake to Configure the Project</strong>:
      <pre><code>cmake .. -DCMAKE_BUILD_TYPE=Release</code></pre>
    </li>

    <li><strong>Compile</strong>:
      <pre><code># For Debug Build:
./compileDebug.sh

# For Release Build:
./compileRelease.sh</code></pre>
    </li>

    <li><strong>Run the Executable</strong>:
      <pre><code>./bin/salamander</code></pre>
    </li>
  </ol>

  <h2>Notes</h2>
  <ul>
    <li>Windows support is not currently provided.</li>
    <li>Ensure Vulkan tools and layers are properly installed to validate the rendering pipeline.</li>
  </ul>

  <hr>

  <p>
    For future development, Salamander aims to evolve into a feature-rich engine suitable 
    for research and learning purposes in real-time graphics and Vulkan API usage.
  </p>

</body>
</html>
