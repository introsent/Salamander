//
// Created by ivans on 14/04/2025.
//

#ifndef SALAMANDER_IBUFFER_H
#define SALAMANDER_IBUFFER_H


#include <vulkan/vulkan.h>

namespace Salamander::Resources::Buffers {
	class IBuffer {
		public:
			virtual ~IBuffer() = default;
			[[nodiscard]] virtual VkBuffer handle() const =0;
	};
}


#endif //SALAMANDER_IBUFFER_H
