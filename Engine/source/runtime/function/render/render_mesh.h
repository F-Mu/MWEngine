#pragma once

#include "rhi/vulkan_resources.h"

namespace MW {
	struct RenderMesh {

		int vertexCount = 0;
		int indexCount = 0;
		VulkanMesh mesh;
	};
}