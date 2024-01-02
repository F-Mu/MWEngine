#include "vulkan_util.h"

namespace MW {
    uint32_t VulkanUtil::alignedSize(uint32_t value, uint32_t alignment) {
        return (value + alignment - 1) & ~(alignment - 1);
    }

    const std::string getAssetPath() {
#if defined(ASSET_DIR)
        return ASSET_DIR;
#else
        return "./../../../../assets/";
#endif
    }

    bool fileExists(const std::string &filename) {
        std::ifstream f(filename.c_str());
        return !f.fail();
    }

    void exitFatal(const std::string &message, int32_t exitCode) {
        std::cerr << message << "\n";
        exit(exitCode);
    }

    void exitFatal(const std::string &message, VkResult resultCode) {
        exitFatal(message, (int32_t) resultCode);
    }
}