# Salamander Renderer Architecture

## Overview

Salamander is a Vulkan-based renderer with a clean, modular architecture inspired by modern AAA game engines (Unreal, Unity, Frostbite). The codebase is organized into well-defined modules with clear separation of concerns.

## Module Structure

```
src/
├── core/                       # Core Vulkan abstractions
│   ├── window.*               # GLFW window management
│   ├── context.*              # Vulkan instance, device, queues
│   ├── swap_chain.*           # Swapchain management
│   ├── image_views.*          # Image view creation
│   ├── framebuffer_manager.*  # Framebuffer management
│   └── debug_messenger.*      # Validation layers
│
├── graphics/                   # Low-level rendering primitives
│   ├── pipeline/              # Graphics & compute pipelines
│   │   ├── pipeline.*         # Graphics pipeline
│   │   ├── compute_pipeline.* # Compute pipeline
│   │   └── push_constants.h   # Push constant structures
│   ├── descriptors/           # Descriptor set management
│   │   ├── descriptor_set_layout.*
│   │   ├── descriptor_set_layout_builder.*
│   │   ├── descriptor_pool_builder.*
│   │   ├── descriptor_manager_base.h
│   │   └── managers/          # Specific descriptor managers
│   │       ├── main_descriptor_manager.*
│   │       └── imgui_descriptor_manager.*
│   ├── render_pass.*          # Render pass (legacy, using dynamic rendering)
│   ├── depth_format.*         # Depth format selection
│   └── image_transition_manager.h  # Image layout transitions
│
├── resources/                  # Resource management
│   ├── buffers/               # Buffer management
│   │   ├── buffer_manager.*   # Buffer creation/allocation
│   │   ├── command_manager.*  # Command buffer submission
│   │   ├── command_buffer.*   # Command buffer wrapper
│   │   ├── command_pool_manager.* # Command pool management
│   │   ├── buffer.h           # Base buffer class
│   │   ├── ibuffer.h          # Buffer interface
│   │   ├── vertex_buffer.*    # Vertex buffer
│   │   ├── index_buffer.*     # Index buffer
│   │   ├── uniform_buffer.*   # Uniform buffer
│   │   └── ssbo_buffer.*      # SSBO buffer
│   ├── textures/              # Texture management
│   │   ├── texture_manager.*  # Texture creation/loading
│   │   ├── texture.*          # Texture wrapper
│   │   └── image.*            # Image resource
│   └── loaders/               # Asset loading
│       ├── assimp_loader.*    # Assimp-based loader
│       └── tinygltf_loader.*  # TinyGLTF loader
│
├── scene/                      # Scene representation
│   ├── components/            # Scene component types
│   │   ├── vertex.h           # Vertex structure
│   │   ├── material.h         # Material & primitive data
│   │   └── transform.h        # Transforms, UBOs, AABB
│   ├── lighting/              # Lighting data
│   │   └── lights.h           # Point & directional lights
│   ├── camera/                # Camera system
│   │   ├── camera.*           # Camera class
│   │   └── camera_exposure.h  # Exposure settings
│   └── scene_data.h           # Main scene data container
│
├── renderer/                   # High-level rendering
│   ├── frame/                 # Frame management
│   │   ├── frame_data.h       # Per-frame resources (Frame struct)
│   │   ├── frame_context.h    # Frame context (index tracking)
│   │   └── render_context.*   # Rendering context (DI container)
│   ├── passes/                # Render passes
│   │   ├── irender_pass.h     # Pass interface
│   │   ├── pass_dependencies.* # Inter-pass dependencies
│   │   ├── depth_prepass.*    # Depth pre-pass
│   │   ├── shadow_pass.*      # Shadow mapping
│   │   ├── gbuffer_pass.*     # G-buffer generation
│   │   ├── lighting_pass.*    # Deferred lighting
│   │   ├── luminance_histogram_pass.* # Histogram generation
│   │   ├── luminance_average_pass.*   # Average luminance
│   │   └── tone_mapping_pass.* # Tone mapping
│   └── targets/               # Render targets
│       ├── render_target.h    # Target interface
│       ├── main_scene_target.* # Main scene rendering
│       ├── main_scene_controller.* # Scene rendering coordinator
│       ├── cube_map_renderer.* # Cubemap generation
│       └── imgui_target.*     # ImGui rendering
│
├── executors/                  # Execution helpers
│   ├── render_pass_executor.h # Pass execution helper
│   └── imgui_pass_executor.*  # ImGui pass executor
│
├── shared/                     # Compatibility headers (deprecated)
│   ├── shared_resources.h     # → Use renderer/frame/render_context.h
│   ├── shared_structs.h       # → Use graphics/pipeline/push_constants.h
│   └── scene_data.h           # → Use scene/scene_data.h
│
├── imgui/                      # Dear ImGui integration
├── includes/                   # Third-party headers (STB, VMA, etc.)
├── renderer.*                  # Main renderer class
├── application.*               # Application entry point
├── main.cpp                    # Program entry
└── deletion_queue.h            # Deferred cleanup queue
```

## Key Design Patterns

### 1. Module Encapsulation

Each module has a clear, single purpose:
- **core**: Vulkan initialization and window management
- **graphics**: Low-level rendering primitives
- **resources**: Asset management and GPU resources
- **scene**: Domain model for the rendered scene
- **renderer**: High-level rendering orchestration

### 2. Dependency Injection

The `RenderContext` class provides type-safe dependency injection:
```cpp
class RenderContext {
    Context& context();
    Window& window();
    SwapChain& swapChain();
    CommandManager& commandManager();
    BufferManager& bufferManager();
    TextureManager& textureManager();
    // ... uses references, not pointers!
};
```

### 3. Interface-Based Design

Render passes implement the `IRenderPass` interface:
```cpp
class IRenderPass {
    virtual void initialize(...) = 0;
    virtual void cleanup() = 0;
    virtual void recreateSwapChain() = 0;
    virtual void execute(VkCommandBuffer cmd, ...) = 0;
};
```

### 4. Resource Lifetime Management

- **Frame Resources**: Managed by `Frame` struct in `renderer/frame/`
- **Scene Resources**: Managed by `MainSceneData` in `scene/`
- **Pass Dependencies**: Tracked explicitly by `PassDependencies`
- **Cleanup**: Deferred cleanup via `DeletionQueue`

### 5. Namespace Organization

Proper C++ namespaces prevent global namespace pollution:
```cpp
namespace Salamander {
    namespace Scene { /* scene types */ }
    namespace Graphics { /* graphics types */ }
    namespace Renderer { /* renderer types */ }
}
```

## Data Flow

### Initialization Flow
```
main()
  └─> VulkanApplication::run()
      └─> createWindowAndContext()
      └─> createAllocator()
      └─> Renderer::Renderer()
          ├─> initializeSharedResources()
          ├─> createCommandBuffers()
          ├─> createSyncObjects()
          └─> Initialize RenderTargets
              └─> MainSceneTarget::initialize()
                  └─> MainSceneController::initialize()
                      ├─> Load model
                      ├─> Create buffers
                      ├─> Create IBL resources
                      └─> Initialize passes
```

### Rendering Flow
```
Renderer::drawFrame()
  ├─> Acquire swapchain image
  ├─> Wait for fence
  ├─> Update uniform buffers
  └─> For each RenderTarget:
      └─> RenderTarget::render()
          └─> MainSceneController::render()
              ├─> Shadow pass
              ├─> Depth prepass
              ├─> G-buffer pass
              ├─> Lighting pass
              ├─> Luminance histogram
              ├─> Luminance average
              └─> Tone mapping pass
  └─> Submit & present
```

## Rendering Pipeline

### Deferred Rendering Pipeline

1. **Shadow Pass** (`shadow_pass.*`)
   - Renders scene from light's perspective
   - Generates shadow map

2. **Depth Prepass** (`depth_prepass.*`)
   - Early depth test
   - Populates depth buffer

3. **G-Buffer Pass** (`gbuffer_pass.*`)
   - Generates geometry buffers:
     - Albedo (RGB)
     - Normal (XYZ)
     - Parameters (metallic, roughness, etc.)

4. **Lighting Pass** (`lighting_pass.*`)
   - Deferred shading
   - Applies lighting using G-buffers
   - Produces HDR output

5. **Luminance Histogram** (`luminance_histogram_pass.*`)
   - Compute shader
   - Generates luminance histogram for auto-exposure

6. **Luminance Average** (`luminance_average_pass.*`)
   - Compute shader
   - Calculates average scene luminance

7. **Tone Mapping** (`tone_mapping_pass.*`)
   - HDR → LDR conversion
   - Applies exposure
   - Final color grading

### IBL (Image-Based Lighting)

- **Equirectangular → Cubemap**: Convert HDR environment map
- **Diffuse Irradiance**: Pre-compute diffuse lighting
- **Specular**: Pre-filtered environment map (planned)

## Key Features

### Current Features
- ✅ Deferred rendering
- ✅ PBR materials (metallic-roughness)
- ✅ Image-based lighting (diffuse irradiance)
- ✅ Cascaded shadow mapping
- ✅ Automatic exposure (histogram-based)
- ✅ HDR rendering with tone mapping
- ✅ GLTF model loading (via Assimp or TinyGLTF)
- ✅ ImGui integration
- ✅ Dynamic rendering (no render passes)

### Architecture Features
- ✅ Modular design
- ✅ Clear separation of concerns
- ✅ Type-safe dependency injection
- ✅ RAII resource management
- ✅ Vulkan Memory Allocator integration
- ✅ Frame-in-flight handling
- ✅ Swapchain recreation support

## Performance Considerations

### Buffer Management
- Uses VMA (Vulkan Memory Allocator) for efficient memory allocation
- Staging buffers for uploads
- Per-frame uniform buffers (double/triple buffered)

### Command Buffer Strategy
- One command buffer per frame in flight
- Records all passes sequentially
- Submits once per frame

### Synchronization
- Semaphores for GPU-GPU sync (image available, render finished)
- Fences for CPU-GPU sync (frame in flight)
- `MAX_FRAMES_IN_FLIGHT = 2` for pipelining

## Future Enhancements

### Planned Architecture Improvements
1. **RenderGraph System**
   - Automatic pass ordering based on dependencies
   - Resource lifetime tracking
   - Barrier insertion
   - Optimization opportunities

2. **FrameGraph**
   - Transient resource allocation
   - Memory aliasing
   - Automatic resource transitions

3. **Component System**
   - ECS (Entity Component System)
   - Better scene management
   - Data-oriented design

### Planned Rendering Features
1. **Specular IBL** - Pre-filtered environment maps
2. **Screen-space reflections** (SSR)
3. **Temporal anti-aliasing** (TAA)
4. **Volumetric lighting**
5. **GPU-driven rendering** - Indirect draws, culling

## Design Principles

### 1. Separation of Concerns
Each module is responsible for one thing:
- Core: Vulkan setup
- Graphics: Low-level primitives
- Resources: Asset management
- Scene: Domain model
- Renderer: Orchestration

### 2. Dependency Inversion
High-level modules don't depend on low-level details:
- Passes depend on interfaces, not concrete implementations
- RenderContext provides abstraction over resources

### 3. Single Responsibility
Each class has one reason to change:
- `Pipeline` - Creates/manages graphics pipelines
- `TextureManager` - Loads/manages textures
- `MainSceneController` - Orchestrates scene rendering

### 4. Explicit is Better Than Implicit
- Pass dependencies are explicit (`PassDependencies`)
- Frame context is explicit (`FrameContext`)
- Resource ownership is clear (unique_ptr, references)

### 5. Composition Over Inheritance
- Render targets compose passes, not inherit from them
- Controllers compose managers and resources

## Comparison to AAA Engines

| Feature | Salamander | Unreal | Unity | Frostbite |
|---------|-----------|--------|-------|-----------|
| Module structure | ✅ | ✅ | ✅ | ✅ |
| RenderGraph | 🚧 | ✅ | ✅ | ✅ |
| PBR | ✅ | ✅ | ✅ | ✅ |
| Deferred rendering | ✅ | ✅ | ✅ | ✅ |
| IBL | ✅ (partial) | ✅ | ✅ | ✅ |
| ECS | ❌ | ✅ | ✅ | ✅ |
| Multi-threading | ❌ | ✅ | ✅ | ✅ |

## Documentation Standards

Each module should have:
- Clear file organization
- Documented public APIs
- Usage examples in comments
- Ownership semantics documented

## Conclusion

Salamander follows modern C++ and rendering engine architecture best practices. The modular design makes it easy to:
- Understand the codebase
- Add new features
- Optimize performance
- Return to the project after time away

The architecture is designed to scale from a learning project to a production-ready renderer.
