#include "render_resource.h"
namespace MW {
    size_t RenderResource::loadTexture(std::string filePath, bool isSrgb) {
        TextureSource textureSource{filePath};
        size_t guid;
        if(textureResources.getElementGuid(textureSource,guid))
            return guid;
        return textureResources.allocGuid(textureSource);
    }

    size_t RenderResource::loadMaterial(MaterialSource materialSource) {

        return 0;
    }

    RenderTexture RenderResource::getTexture(size_t id) {
        return RenderTexture();
    }

    RenderMaterial RenderResource::getMaterial(size_t id) {
        return RenderMaterial();
    }
}