#pragma once
#include <cstdint>

namespace Salamander::Renderer::Frame {
    /// FrameContext holds per-frame state information that changes each frame during rendering
    class FrameContext {
    public:
        FrameContext(const uint32_t frameIndex, const uint32_t imageIndex)
            : m_frameIndex(frameIndex)
            , m_imageIndex(imageIndex)
        {
        }

        [[nodiscard]] uint32_t frameIndex() const { return m_frameIndex; }
        [[nodiscard]] uint32_t imageIndex() const { return m_imageIndex; }

    private:
        uint32_t m_frameIndex;
        uint32_t m_imageIndex;
    };
}
