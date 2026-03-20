#pragma once
#include <vulkan/vulkan.h>

namespace Salamander::Resources::Buffers {
	class IBuffer {
		public:
			virtual ~IBuffer() = default;
			[[nodiscard]] virtual VkBuffer handle() const =0;
	};
}
