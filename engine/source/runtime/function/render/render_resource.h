#pragma once

#include "core/math/math_headers.h"
#include "runtime/function/render/render_guid_allocator.h"
#include "runtime/function/render/render_type.h"
#include "render_entity.h"
#include <vulkan/vulkan.h>
#include <array>
#include <cstdint>
#include <map>
#include <vector>
#include <cmath>
#include <string>

namespace MW {
    class RenderResource {
    public:
        CameraObject cameraObject;
        CameraUVWObject cameraUVWObject;

        GuidAllocator<MaterialSource> materialResources;
        GuidAllocator<TextureSource> textureResources;

        size_t loadTexture(std::string filePath, bool isSrgb = false);

        size_t loadMaterial(MaterialSource materialSource);

        RenderTexture getTexture(size_t id);

        RenderMaterial getMaterial(size_t id);
    };
} // namespace MW
