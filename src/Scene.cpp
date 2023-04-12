#include "Scene.h"

#include "DebugUtils.h"

namespace VKRT {

Scene::Scene(Context* context)
    : mContext(context),
      mObjects(),
      mInstanceBuffer(nullptr),
      mTLASBuffer(nullptr),
      mCommitted(false) {
    context->AddRef();
}

void Scene::AddObject(Object* object) {
    VKRT_ASSERT(!mCommitted);
    if (object != nullptr) {
        mObjects.emplace_back(object);
    }
}

void Scene::AddLight(Light* light) {
    if (light != nullptr) {
        mLights.emplace_back(light);
    }
}

std::vector<Model::Description> Scene::GetDescriptions() {
    std::vector<Model::Description> descriptions;
    for (const Object* object : mObjects) {
        descriptions.emplace_back(object->GetModel()->GetDescription());
    }
    return descriptions;
}

std::vector<Light::Description> Scene::GetLightDescriptions() {
    std::vector<Light::Description> descriptions;
    for (Light* light : mLights) {
        Light::Description description{
            .position = light->GetPosition(),
            .intensity = light->GetIntensity()};
        descriptions.emplace_back(description);
    }
    return descriptions;
}

Scene::SceneMaterials Scene::GetMaterialProxies() {
    // Gather textures first
    std::vector<std::pair<const Texture*, int32_t>> textureIndices;
    int32_t currentTextureIndex = 0;
    for (const Object* object : mObjects) {
        const Material* material = object->GetModel()->GetMaterial();
        {
            const Texture* albedoTexture = material->GetAlbedoTexture();
            if (albedoTexture != nullptr) {
                auto albedoIt = std::find_if(
                    textureIndices.begin(),
                    textureIndices.end(),
                    [&albedoTexture](const auto& entry) { return entry.first == albedoTexture; });

                if (albedoIt == textureIndices.end()) {
                    textureIndices.emplace_back(albedoTexture, currentTextureIndex);
                    ++currentTextureIndex;
                }
            }
        }
        {
            const Texture* roughnessTexture = material->GetRoughnessTexture();
            if (roughnessTexture != nullptr) {
                auto roughnessIt = std::find_if(
                    textureIndices.begin(),
                    textureIndices.end(),
                    [&roughnessTexture](const auto& entry) {
                        return entry.first == roughnessTexture;
                    });
                if (roughnessIt == textureIndices.end()) {
                    textureIndices.emplace_back(roughnessTexture, currentTextureIndex);
                    ++currentTextureIndex;
                }
            }
        }
    }

    // Gather materials and generate texture indices if applicable
    std::vector<MaterialProxy> materials;
    for (const Object* object : mObjects) {
        const Material* material = object->GetModel()->GetMaterial();
        MaterialProxy proxy{
            .albedo = material->GetAlbedo(),
            .roughness = material->GetRoughness(),
            .albedoTextureIndex = -1,
            .roughnessTextureIndex = -1,
        };
        {
            const Texture* albedoTexture = material->GetAlbedoTexture();
            auto albedoIt = std::find_if(
                textureIndices.begin(),
                textureIndices.end(),
                [&albedoTexture](const auto& entry) { return entry.first == albedoTexture; });
            if (albedoIt != textureIndices.end()) {
                proxy.albedoTextureIndex = albedoIt->second;
            }
        }
        {
            const Texture* roughnessTexture = material->GetRoughnessTexture();
            auto roughnessIt = std::find_if(
                textureIndices.begin(),
                textureIndices.end(),
                [&roughnessTexture](const auto& entry) { return entry.first == roughnessTexture; });
            if (roughnessIt != textureIndices.end()) {
                proxy.roughnessTextureIndex = roughnessIt->second;
            }
        }
        materials.push_back(proxy);
    }

    std::vector<const Texture*> textures;
    for (const auto& entry : textureIndices) {
        textures.push_back(entry.first);
    }

    Scene::SceneMaterials sceneMaterials{
        .materials = materials,
        .textures = textures,
    };
    return sceneMaterials;
}

void Scene::Commit() {
    VKRT_ASSERT(!mCommitted);
    if (!mObjects.empty() && !mCommitted) {
        mCommitted = true;
        std::vector<vk::AccelerationStructureInstanceKHR> instances;
        uint32_t index = 0;
        for (Object* object : mObjects) {
            const glm::mat4& transform = glm::transpose(object->GetTransform());
            VkTransformMatrixKHR transformMatrix =
                *(reinterpret_cast<const VkTransformMatrixKHR*>(&transform));
            instances.emplace_back(
                vk::AccelerationStructureInstanceKHR()
                    .setTransform(transformMatrix)
                    .setInstanceCustomIndex(index)
                    .setAccelerationStructureReference(object->GetModel()->GetBLASAddress())
                    .setMask(0xFF)
                    .setInstanceShaderBindingTableRecordOffset(0)
                    .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable));
            ++index;
        }

        const size_t instanceDataSize =
            instances.size() * sizeof(vk::AccelerationStructureInstanceKHR);
        mInstanceBuffer = mContext->GetDevice()->CreateBuffer(
            instanceDataSize,
            vk::BufferUsageFlagBits::eShaderDeviceAddress |
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* instanceData = mInstanceBuffer->MapBuffer();
        std::copy_n(reinterpret_cast<uint8_t*>(instances.data()), instanceDataSize, instanceData);
        mInstanceBuffer->UnmapBuffer();
        const vk::DeviceAddress instanceBufferAddress = mInstanceBuffer->GetDeviceAddress();

        vk::AccelerationStructureGeometryInstancesDataKHR instancesData =
            vk::AccelerationStructureGeometryInstancesDataKHR().setArrayOfPointers(false).setData(
                instanceBufferAddress);
        vk::AccelerationStructureGeometryKHR accelerationStructureGeometry =
            vk::AccelerationStructureGeometryKHR()
                .setGeometryType(vk::GeometryTypeKHR::eInstances)
                .setFlags(vk::GeometryFlagBitsKHR::eOpaque)
                .setGeometry(instancesData);

        vk::AccelerationStructureBuildGeometryInfoKHR accelerationStructureBuildGeometryInfo =
            vk::AccelerationStructureBuildGeometryInfoKHR()
                .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
                .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
                .setGeometries(accelerationStructureGeometry);

        uint32_t instanceCount = static_cast<uint32_t>(instances.size());
        vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
        vk::AccelerationStructureBuildSizesInfoKHR buildSizesInfo =
            logicalDevice.getAccelerationStructureBuildSizesKHR(
                vk::AccelerationStructureBuildTypeKHR::eDevice,
                accelerationStructureBuildGeometryInfo,
                instanceCount,
                mContext->GetDevice()->GetDispatcher());

        mTLASBuffer = mContext->GetDevice()->CreateBuffer(
            buildSizesInfo.accelerationStructureSize,
            vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR |
                vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::MemoryAllocateFlagBits::eDeviceAddress);

        vk::AccelerationStructureCreateInfoKHR accelerationStructureCreateInfo =
            vk::AccelerationStructureCreateInfoKHR()
                .setBuffer(mTLASBuffer->GetBufferHandle())
                .setSize(buildSizesInfo.accelerationStructureSize)
                .setType(vk::AccelerationStructureTypeKHR::eTopLevel);
        mTLAS = VKRT_ASSERT_VK(logicalDevice.createAccelerationStructureKHR(
            accelerationStructureCreateInfo,
            nullptr,
            mContext->GetDevice()->GetDispatcher()));

        VulkanBuffer* scratchBuffer = mContext->GetDevice()->CreateBuffer(
            buildSizesInfo.buildScratchSize,
            vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eDeviceLocal,
            vk::MemoryAllocateFlagBits::eDeviceAddress);

        vk::AccelerationStructureBuildGeometryInfoKHR accelerationBuildGeometryInfo =
            vk::AccelerationStructureBuildGeometryInfoKHR()
                .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
                .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace)
                .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
                .setDstAccelerationStructure(mTLAS)
                .setGeometries(accelerationStructureGeometry)
                .setScratchData(scratchBuffer->GetDeviceAddress());

        vk::AccelerationStructureBuildRangeInfoKHR accelerationStructureBuildRangeInfo =
            vk::AccelerationStructureBuildRangeInfoKHR()
                .setPrimitiveCount(instanceCount)
                .setPrimitiveOffset(0)
                .setFirstVertex(0)
                .setTransformOffset(0);
        vk::CommandBuffer commandBuffer = mContext->GetDevice()->CreateCommandBuffer();
        VKRT_ASSERT_VK(commandBuffer.begin(vk::CommandBufferBeginInfo{}));
        commandBuffer.buildAccelerationStructuresKHR(
            accelerationBuildGeometryInfo,
            &accelerationStructureBuildRangeInfo,
            mContext->GetDevice()->GetDispatcher());
        VKRT_ASSERT_VK(commandBuffer.end());
        mContext->GetDevice()->SubmitCommandAndFlush(commandBuffer);
        mContext->GetDevice()->DestroyCommand(commandBuffer);

        vk::AccelerationStructureDeviceAddressInfoKHR accelerationDeviceAddressInfo =
            vk::AccelerationStructureDeviceAddressInfoKHR().setAccelerationStructure(mTLAS);
        mTLASAddress = logicalDevice.getAccelerationStructureAddressKHR(
            accelerationDeviceAddressInfo,
            mContext->GetDevice()->GetDispatcher());

        scratchBuffer->Release();
    }
}

Scene::~Scene() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    if (mCommitted) {
        logicalDevice.destroyAccelerationStructureKHR(
            mTLAS,
            nullptr,
            mContext->GetDevice()->GetDispatcher());
        mTLASBuffer->Release();
        mInstanceBuffer->Release();
    }
    for (Object* object : mObjects) {
        object->Release();
    }
    mContext->Release();
}

}  // namespace VKRT