#include "vulkan_util.h"

namespace MW {
    uint32_t VulkanUtil::alignedSize(uint32_t value, uint32_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }
}