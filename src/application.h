#pragma once

#include <string>
#include "core/data_structures.h"
#include "core/window.h"
#include "core/context.h"
#include "renderer.h"


constexpr uint32_t WIDTH = 800;
constexpr uint32_t HEIGHT = 600;
const std::string MODEL_PATH = "./models/viking_room.obj";
const std::string TEXTURE_PATH = "./textures/viking_room.png";

class VulkanApplication {
public:
    void run();

private:
    Window* m_window = nullptr;
    Context* m_context = nullptr;
    Renderer* m_renderer = nullptr;
    VmaAllocator allocator = VK_NULL_HANDLE;

    static bool enableValidationLayers() {
#ifdef NDEBUG
        return false;
#else
        return true;
#endif
    }

    void createWindowAndContext();
    void createAllocator();
    void mainLoop() const;
    void cleanup() const;
};
