#pragma once

#include <vector>

#include "Object.h"
#include "RefCountPtr.h"
#include "VulkanBase.h"
#include "VulkanBuffer.h"

namespace VKRT {

class Context;

class Scene : public RefCountPtr {
public:
    Scene(ScopedRefPtr<Context> context);

    void AddObject(ScopedRefPtr<Object> object);

    const vk::AccelerationStructureKHR& GetTLAS() const { return mTLAS; }

    std::vector<Mesh::Description> GetDescriptions();

    struct MaterialProxy {
        glm::vec3 albedo;
        glm::vec3 emissive;
        float roughness;
        float metallic;
        float transmission;
        float indexOfRefraction;
        int32_t albedoTextureIndex;
        int32_t roughnessTextureIndex;
    };
    struct SceneMaterials {
        std::vector<MaterialProxy> materials;
        std::vector<ScopedRefPtr<Texture>> textures;
    };
    SceneMaterials GetMaterialProxies();

    void Update(vk::CommandBuffer& commandBuffer);

    ~Scene();

private:
    ScopedRefPtr<Context> mContext;

    std::vector<ScopedRefPtr<Object>> mObjects;

    ScopedRefPtr<VulkanBuffer> mInstanceBuffer;
    ScopedRefPtr<VulkanBuffer> mTLASBuffer;
    ScopedRefPtr<VulkanBuffer> mScratchBuffer;
    vk::AccelerationStructureKHR mTLAS;
    vk::DeviceAddress mTLASAddress;

    ScopedRefPtr<Texture> mDummyTexture;
};
}  // namespace VKRT