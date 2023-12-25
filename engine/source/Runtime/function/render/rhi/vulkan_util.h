#pragma once

#include <iostream>

namespace MW {
    class VulkanUtil {
    public:
        static uint32_t alignedSize(uint32_t value, uint32_t alignment);
    };
} // namespace MW
