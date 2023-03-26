#include "RayTracingPipeline.h"

#include "Context.h"
#include "DebugUtils.h"

enum class RayTracingStage { Generate = 0, Hit, Miss };

namespace VKRT {
RayTracingPipeline::RayTracingPipeline(Context* context) : mContext(context) {
    mContext->AddRef();

    vk::DescriptorSetLayoutBinding accelerationStructureLayoutBinding = vk::DescriptorSetLayoutBinding()
                                                                            .setBinding(0)
                                                                            .setDescriptorType(vk::DescriptorType::eAccelerationStructureKHR)
                                                                            .setDescriptorCount(1)
                                                                            .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

    vk::DescriptorSetLayoutBinding resultImageLayoutBinding = vk::DescriptorSetLayoutBinding()
                                                                  .setBinding(1)
                                                                  .setDescriptorType(vk::DescriptorType::eStorageImage)
                                                                  .setDescriptorCount(1)
                                                                  .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

    vk::DescriptorSetLayoutBinding uniformBufferBinding = vk::DescriptorSetLayoutBinding()
                                                              .setBinding(2)
                                                              .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                                                              .setDescriptorCount(1)
                                                              .setStageFlags(vk::ShaderStageFlagBits::eRaygenKHR);

    std::vector<vk::DescriptorSetLayoutBinding> descriptorBindings{
        accelerationStructureLayoutBinding,
        resultImageLayoutBinding,
        uniformBufferBinding,
    };
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();

    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = vk::DescriptorSetLayoutCreateInfo().setBindings(descriptorBindings);
    mDescriptorLayout = VKRT_ASSERT_VK(logicalDevice.createDescriptorSetLayout(descriptorSetLayoutCreateInfo));

    mShaders = std::vector<vk::ShaderModule>{
        LoadShader(Resource::Id::GenShader),
        LoadShader(Resource::Id::HitShader),
        LoadShader(Resource::Id::MissShader),
    };

    std::vector<vk::PipelineShaderStageCreateInfo> stageCreateInfos{
        vk::PipelineShaderStageCreateInfo()
            .setPName("main")
            .setModule(mShaders.at(static_cast<uint32_t>(RayTracingStage::Generate)))
            .setStage(vk::ShaderStageFlagBits::eRaygenKHR),
        vk::PipelineShaderStageCreateInfo()
            .setPName("main")
            .setModule(mShaders.at(static_cast<uint32_t>(RayTracingStage::Hit)))
            .setStage(vk::ShaderStageFlagBits::eClosestHitKHR),
        vk::PipelineShaderStageCreateInfo()
            .setPName("main")
            .setModule(mShaders.at(static_cast<uint32_t>(RayTracingStage::Miss)))
            .setStage(vk::ShaderStageFlagBits::eMissKHR)};

    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> rayTracingGroupCreateInfos{
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR)
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(static_cast<uint32_t>(RayTracingStage::Generate)),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR)
            .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
            .setClosestHitShader(static_cast<uint32_t>(RayTracingStage::Hit)),
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setAnyHitShader(VK_SHADER_UNUSED_KHR)
            .setClosestHitShader(VK_SHADER_UNUSED_KHR)
            .setGeneralShader(VK_SHADER_UNUSED_KHR)
            .setIntersectionShader(VK_SHADER_UNUSED_KHR)
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(static_cast<uint32_t>(RayTracingStage::Miss)),
    };

    vk::PipelineLayoutCreateInfo layoutCreateInfo = vk::PipelineLayoutCreateInfo().setSetLayouts(mDescriptorLayout);
    mLayout = VKRT_ASSERT_VK(logicalDevice.createPipelineLayout(layoutCreateInfo));

    vk::RayTracingPipelineCreateInfoKHR rayTracingPipelineCreateInfo = vk::RayTracingPipelineCreateInfoKHR()
                                                                           .setStages(stageCreateInfos)
                                                                           .setGroups(rayTracingGroupCreateInfos)
                                                                           .setMaxPipelineRayRecursionDepth(1)
                                                                           .setLayout(mLayout);
    mPipeline = VKRT_ASSERT_VK(
        logicalDevice.createRayTracingPipelineKHR({}, {}, rayTracingPipelineCreateInfo, nullptr, mContext->GetDevice()->GetDispatcher()));

    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR rayTracingProperties = mContext->GetDevice()->GetRayTracingProperties();
    const size_t handleSize = rayTracingProperties.shaderGroupHandleSize;
    const size_t handleAlignment = rayTracingProperties.shaderGroupHandleAlignment;
    const size_t handleSizeAligned = (handleSize + handleAlignment - 1) & ~(handleAlignment - 1);
    const uint32_t groupCount = static_cast<uint32_t>(rayTracingGroupCreateInfos.size());
    const size_t sbtSize = groupCount * handleSizeAligned;

    std::vector<uint8_t> shaderHandleStorage = VKRT_ASSERT_VK(
        logicalDevice.getRayTracingShaderGroupHandlesKHR<uint8_t>(mPipeline, 0, groupCount, sbtSize, mContext->GetDevice()->GetDispatcher()));

    {
        mRayGenTable = mContext->GetDevice()->CreateBuffer(
            handleSize,
            vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* rayGenTableData = mRayGenTable->MapBuffer();
        std::copy_n(shaderHandleStorage.begin(), handleSize, rayGenTableData);
        mRayGenTable->UnmapBuffer();
    }

    {
        mRayHitTable = mContext->GetDevice()->CreateBuffer(
            handleSize,
            vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* rayHitTableData = mRayHitTable->MapBuffer();
        std::copy_n(shaderHandleStorage.begin() + handleSizeAligned, handleSize, rayHitTableData);
        mRayHitTable->UnmapBuffer();
    }

    {
        mRayMissTable = mContext->GetDevice()->CreateBuffer(
            handleSize,
            vk::BufferUsageFlagBits::eShaderBindingTableKHR | vk::BufferUsageFlagBits::eShaderDeviceAddress,
            vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent,
            vk::MemoryAllocateFlagBits::eDeviceAddress);
        uint8_t* rayMissTableData = mRayMissTable->MapBuffer();
        std::copy_n(shaderHandleStorage.begin() + handleSizeAligned * 2, handleSize, rayMissTableData);
        mRayMissTable->UnmapBuffer();
    }
}

const std::vector<vk::DescriptorPoolSize>& RayTracingPipeline::GetDescriptorSizes() const {
    static std::vector<vk::DescriptorPoolSize> poolSizes {
        vk::DescriptorPoolSize(vk::DescriptorType::eAccelerationStructureKHR, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eStorageImage, 1),
        vk::DescriptorPoolSize(vk::DescriptorType::eUniformBuffer, 1),
    };
    return poolSizes;
}

vk::ShaderModule RayTracingPipeline::LoadShader(Resource::Id shaderId) {
    Resource shaderResource = ResourceLoader::Load(shaderId);
    vk::ShaderModuleCreateInfo shaderCreateInfo = vk::ShaderModuleCreateInfo()
                                                      .setCodeSize(shaderResource.size * sizeof(uint8_t))
                                                      .setPCode(reinterpret_cast<const uint32_t*>(shaderResource.buffer));
    vk::ShaderModule shaderModule = VKRT_ASSERT_VK(mContext->GetDevice()->GetLogicalDevice().createShaderModule(shaderCreateInfo));
    ResourceLoader::CleanUp(shaderResource);
    return shaderModule;
}

RayTracingPipeline::~RayTracingPipeline() {
    vk::Device& logicalDevice = mContext->GetDevice()->GetLogicalDevice();
    mRayGenTable->Release();
    mRayHitTable->Release();
    mRayMissTable->Release();
    for (vk::ShaderModule& shader : mShaders) {
        logicalDevice.destroyShaderModule(shader);
    }
    logicalDevice.destroyPipeline(mPipeline);
    logicalDevice.destroyPipelineLayout(mLayout);
    mContext->Release();
}
}  // namespace VKRT