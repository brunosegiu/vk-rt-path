#pragma once

#include "Camera.h"
#include "Context.h"
#include "Pipeline.h"
#include "ProbeGrid.h"
#include "RefCountPtr.h"
#include "Scene.h"

namespace VKRT {
class Renderer : public RefCountPtr {
public:
    Renderer(ScopedRefPtr<Context> context, ScopedRefPtr<Scene> scene);

    void Render(Camera* camera);

    ~Renderer();

private:
    void CreateStorageImage();
    void CreateUniformBuffer();
    void CreateMaterialUniforms();
    void CreateDescriptors(const Scene::SceneMaterials& materialInfo);
    void UpdateDescriptors(const Scene::SceneMaterials& materialInfo);
    struct UniformData {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
        uint32_t framesSinceMoved;
        uint32_t randomSeed;
    };

    void UpdateCameraUniforms(Camera* camera);
    void UpdateMaterialUniforms(const Scene::SceneMaterials& materialInfo);

    ScopedRefPtr<Context> mContext;
    ScopedRefPtr<Scene> mScene;

    ScopedRefPtr<Texture> mStorageTexture;

    ScopedRefPtr<VulkanBuffer> mCameraUniformBuffer;
    ScopedRefPtr<VulkanBuffer> mSceneUniformBuffer;
    ScopedRefPtr<VulkanBuffer> mMaterialsBuffer;

    ScopedRefPtr<Pipeline> mMainPassPipeline;
    vk::DescriptorPool mDescriptorPool;
    vk::DescriptorSet mDescriptorSet;

    vk::Sampler mTextureSampler;
};

}  // namespace VKRT