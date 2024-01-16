#pragma once

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "core/math/math_headers.h"
#include "core/base/hash.h"
#include <glm/glm.hpp>
#include <string>

namespace MW {
    constexpr uint16_t maxLightsCount = 4;
    struct CameraObject {
        glm::mat4 projMatrix;
        glm::mat4 viewMatrix;
        glm::vec4 directionalLightPos = glm::vec4(0.0f, 2.5f, 0.0f, 1.0f);
        glm::vec4 viewPos;
        glm::mat4 projViewMatrix;
        glm::vec3 position;
        glm::vec4 lights[maxLightsCount];
    };
    struct CameraRTData {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
        glm::vec4 lightPos;
        int32_t vertexSize;
    };
//    struct CameraUVWObject {
//        Vector3 camera_position;
//        float _padding_camera_pos;
//        Vector3 camera_U;
//        float _padding_camera_U;
//        Vector3 camera_V;
//        float _padding_camera_V;
//        Vector3 camera_W;
//        float _padding_camera_W;
//        Vector4 lightPos;
//        int32_t vertexSize;
//    };

    struct MaterialSource {
        std::string baseColorFile;
        std::string metallicRoughnessFile;
        std::string normalFile;
        std::string occlusionFile;
        std::string emissiveFile;

        bool operator==(const MaterialSource &rhs) const {
            return baseColorFile == rhs.baseColorFile &&
                   metallicRoughnessFile == rhs.metallicRoughnessFile &&
                   normalFile == rhs.normalFile &&
                   occlusionFile == rhs.occlusionFile &&
                   emissiveFile == rhs.emissiveFile;
        }

        size_t getHashValue() const {
            size_t hash = 0;
            hash_combine(hash,
                         baseColorFile,
                         metallicRoughnessFile,
                         normalFile,
                         occlusionFile,
                         emissiveFile);
            return hash;
        }
    };

    struct TextureSource {
        std::string textureFile;

        bool operator==(const TextureSource &rhs) const { return textureFile == rhs.textureFile; }

        size_t getHashValue() const { return std::hash<std::string>{}(textureFile); }
    };
}

template<>
struct std::hash<MW::MaterialSource> {
    size_t operator()(const MW::MaterialSource &rhs) const noexcept { return rhs.getHashValue(); }
};

template<>
struct std::hash<MW::TextureSource> {
    size_t operator()(const MW::TextureSource &rhs) const noexcept { return rhs.getHashValue(); }
};