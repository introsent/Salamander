# Salamander Renderer - Refactoring Guide

## Overview
This guide documents the architectural refactoring completed and provides instructions for fixing remaining compilation issues.

## What Was Changed

### 1. Directory Reorganization

**Old Structure → New Structure:**
```
src/rendering/              → src/graphics/
src/rendering/camera/       → src/scene/camera/
src/rendering/descriptors/  → src/graphics/descriptors/
src/rendering/pipeline.*    → src/graphics/pipeline/pipeline.*
src/rendering/compute_pipeline.* → src/graphics/pipeline/compute_pipeline.*
src/rendering/target/       → src/renderer/targets/

src/resources/buffer.*      → src/resources/buffers/
src/resources/texture.*     → src/resources/textures/
src/resources/command_*     → src/resources/buffers/

src/user/user_passes/       → src/renderer/passes/
src/user/user_render_targets/ → src/renderer/targets/
src/user/user_descriptor_managers/ → src/graphics/descriptors/managers/
src/user/user_executors/    → src/executors/
```

### 2. File Reorganization

**Eliminated "dumping ground" files:**
- `src/core/data_structures.h` → Split into domain modules
  - `Vertex` → `src/scene/components/vertex.h`
  - `UniformBufferObject`, `AABB` → `src/scene/components/transform.h`
  - `RenderObject`, `GLTFPrimitiveData` → `src/scene/components/material.h`
  - `Frame`, `MAX_FRAMES_IN_FLIGHT` → `src/renderer/frame/frame_data.h`
  - `PointLightData`, `DirectionalLightData` → `src/scene/lighting/lights.h`
  - `CameraExposure` → `src/scene/camera/camera_exposure.h`

- `src/shared/shared_structs.h` → `src/graphics/pipeline/push_constants.h`
  - `PushConstants`, `TonePush`, `ShadowPushConstants` moved

- `src/shared/scene_data.h` → Split:
  - `MainSceneGlobalData` → `src/scene/scene_data.h` (renamed to `MainSceneData`)
  - `PassDependencies` → `src/renderer/passes/pass_dependencies.h`
  - `globalScale` → Now member of `MainSceneData::modelScale`

- `src/shared/shared_resources.h` → `src/renderer/frame/render_context.h`
  - Replaced raw pointer struct with proper RAII class
  - Uses references instead of pointers

### 3. Namespace Organization

All new code uses proper namespaces:
```cpp
namespace Salamander {
    namespace Scene {
        struct Vertex;
        struct MainSceneData;
        struct DirectionalLightData;
        // ...
    }

    namespace Graphics {
        struct PushConstants;
        // ...
    }

    namespace Renderer {
        struct Frame;
        struct PassDependencies;
        constexpr int MAX_FRAMES_IN_FLIGHT = 2;
        // ...
    }
}
```

### 4. New Abstractions Created

- **`RenderContext`** (`src/renderer/frame/render_context.h`)
  - Replaces SharedResources with proper C++ design
  - Uses references instead of raw pointers
  - Type-safe dependency injection

- **`FrameContext`** (`src/renderer/frame/frame_context.h`)
  - Encapsulates per-frame state (frameIndex, imageIndex)
  - Makes frame context explicit

## Required Updates

### Step 1: Update All Pass Headers

For each file in `src/renderer/passes/*.h`:

**Replace these includes:**
```cpp
// OLD
#include "render_pass.h"
#include "pipeline.h"
#include "descriptors/descriptor_set_layout.h"
#include "user_descriptor_managers/main_descriptor_manager.h"
#include "shared/scene_data.h"

// NEW
#include "graphics/render_pass.h"
#include "graphics/pipeline/pipeline.h"
#include "graphics/pipeline/compute_pipeline.h"  // if needed
#include "graphics/descriptors/descriptor_set_layout.h"
#include "graphics/descriptors/managers/main_descriptor_manager.h"
#include "renderer/frame/frame_data.h"
```

**Update method signatures:**
```cpp
// OLD
void initialize(const SharedResources& shared,
               MainSceneGlobalData& globalData,
               PassDependencies& dependencies) override;

// NEW
void initialize(const SharedResources& shared,
               Salamander::Scene::MainSceneData& globalData,
               Salamander::Renderer::PassDependencies& dependencies) override;
```

**Update member variable types:**
```cpp
// OLD
MainSceneGlobalData* m_globalData = nullptr;
PassDependencies* m_dependencies = nullptr;
std::array<Texture*, MAX_FRAMES_IN_FLIGHT> m_textures;

// NEW
Salamander::Scene::MainSceneData* m_globalData = nullptr;
Salamander::Renderer::PassDependencies* m_dependencies = nullptr;
std::array<Texture*, Salamander::Renderer::MAX_FRAMES_IN_FLIGHT> m_textures;
```

### Step 2: Update All Pass CPP Files

For each file in `src/renderer/passes/*.cpp`:

**Update includes:**
```cpp
// Add these at the top
#include "shared/shared_resources.h"  // Temporary compatibility
#include "renderer/passes/pass_dependencies.h"
#include "scene/scene_data.h"
#include "graphics/pipeline/pipeline.h"
#include "graphics/descriptors/descriptor_set_layout_builder.h"
```

**Replace `globalScale` usage:**
```cpp
// OLD
.modelScale = globalScale

// NEW
.modelScale = m_globalData->modelScale
```

**Files requiring this change:**
- `depth_prepass.cpp` (line ~103)
- `gbuffer_pass.cpp` (line ~171)  ✅ DONE
- `shadow_pass.cpp` (line ~108)

### Step 3: Update Loader Files

In `src/resources/loaders/tinygltf_loader.cpp`:

**Replace global scale usage:**
```cpp
// OLD (line ~58)
v.pos = glm::make_vec3(&positions[3 * i]) * globalScale;

// OLD (line ~122)
globalScale = glm::vec3(...);

// NEW - Pass sceneData reference and use:
sceneData.modelScale = glm::vec3(...);
v.pos = glm::make_vec3(&positions[3 * i]) * sceneData.modelScale;
```

### Step 4: Update Render Targets

**Files:** `src/renderer/targets/*.h`

**Update includes:**
```cpp
// OLD
#include "target/render_target.h"
#include "shared/scene_data.h"

// NEW
#include "renderer/targets/render_target.h"
#include "scene/scene_data.h"
#include "renderer/passes/pass_dependencies.h"
```

### Step 5: Update Main Controller

**File:** `src/renderer/targets/main_scene_controller.h`

**Update includes:**
```cpp
// OLD
#include "user_passes/depth_prepass.h"
#include "user_passes/gbuffer_pass.h"
// ...

// NEW
#include "renderer/passes/depth_prepass.h"
#include "renderer/passes/gbuffer_pass.h"
#include "renderer/passes/lighting_pass.h"
#include "renderer/passes/tone_mapping_pass.h"
#include "renderer/passes/shadow_pass.h"
#include "renderer/passes/luminance_histogram_pass.h"
#include "renderer/passes/luminance_average_pass.h"
```

**Update member types:**
```cpp
// OLD
MainSceneGlobalData m_globalData;
PassDependencies m_dependencies;

// NEW
Salamander::Scene::MainSceneData m_globalData;
Salamander::Renderer::PassDependencies m_dependencies;
```

### Step 6: Update Application & Renderer

**Files:**
- `src/application.h` - Update includes for `camera.h` → `scene/camera/camera.h`
- `src/renderer.h` - Update includes for relocated files
- `src/renderer.cpp` - Update target includes

## Backward Compatibility

The old files (`data_structures.h`, `shared_structs.h`, etc.) are still present as **compatibility headers** that:
1. Forward-declare the new types
2. Re-export them to the global namespace
3. Provide deprecation warnings
4. Include migration instructions

This allows gradual migration. Eventually, these compatibility headers should be deleted.

## Global Variables Removed

**Inline globals that need to be class members:**

1. **`globalScale`** - Now `MainSceneData::modelScale`
   - Already migrated ✅

2. **`directionalLight`** - Should be a member variable
   - Location: Previously in `data_structures.h:80`
   - TODO: Add to `main_scene_controller.h` as member
   - Type: `Salamander::Scene::DirectionalLightData`

3. **`camExpUBO`** - Should be a member variable
   - Location: Previously in `data_structures.h:89`
   - TODO: Add to `main_scene_controller.h` as member
   - Type: `Salamander::Scene::CameraExposure`

## Compilation Quick Fixes

### Common Error #1: Cannot find header
```
fatal error: 'render_pass.h' file not found
```
**Fix:** Add the new path prefix: `graphics/render_pass.h`

### Common Error #2: Unknown type name
```
error: unknown type name 'MainSceneGlobalData'
```
**Fix:** Use fully qualified name or add using directive:
```cpp
using MainSceneGlobalData = Salamander::Scene::MainSceneData;
```

### Common Error #3: globalScale not found
```
error: use of undeclared identifier 'globalScale'
```
**Fix:** Replace with `m_globalData->modelScale`

### Common Error #4: MAX_FRAMES_IN_FLIGHT not found
```
error: use of undeclared identifier 'MAX_FRAMES_IN_FLIGHT'
```
**Fix:** Use `Salamander::Renderer::MAX_FRAMES_IN_FLIGHT` or include `renderer/frame/frame_data.h`

## Testing Checklist

After compilation succeeds:

- [ ] Test application launches
- [ ] Test rendering works correctly
- [ ] Test swapchain recreation
- [ ] Test ImGui rendering
- [ ] Test all render passes execute
- [ ] Test shadow mapping
- [ ] Test tone mapping
- [ ] Test camera movement
- [ ] Test model loading

## Future Work

1. **Convert SharedResources to RenderContext** (major refactor)
   - Update all passes to use `RenderContext` instead
   - Remove the compatibility `SharedResources` struct

2. **Add RenderGraph system**
   - Automatic pass dependency management
   - Resource lifetime tracking

3. **Remove compatibility headers**
   - Delete `src/core/data_structures.h`
   - Delete `src/shared/*.h` files

4. **Add CMake organization**
   - Group files by module in CMakeLists.txt
   - Create library targets for each module

## Benefits Achieved

✅ **Clear module boundaries** - Each directory has a single, clear purpose
✅ **No more "dumping ground" files** - Everything has a logical home
✅ **Better discoverability** - File location reveals purpose
✅ **Namespace organization** - Reduced global namespace pollution
✅ **Professional architecture** - Similar to AAA game engines
✅ **Easy to navigate** - Clear even after time away from project

## Questions?

If you encounter issues:
1. Check this guide for the specific error
2. Look at `gbuffer_pass.h/cpp` as a reference (already updated)
3. Check the compatibility headers for type mappings
4. Use the IDE's "Find Usages" to locate all references to old names
