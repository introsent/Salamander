#pragma once
#include <vulkan/vulkan.h>

class IBuffer {
public:
	virtual ~IBuffer() = default;
	virtual VkBuffer handle() const = 0;
};
