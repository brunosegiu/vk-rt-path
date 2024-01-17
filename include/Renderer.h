#pragma once

#include "Camera.h"
#include "Context.h"
#include "Pipeline.h"
#include "ProbeGrid.h"
#include "RefCountPtr.h"
#include "Scene.h"

namespace VKRT {
class Renderer : public RefCountPtr, public InputEventListener {
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
    struct CameraProperties {
        glm::mat4 viewInverse;
        glm::mat4 projInverse;
        uint32_t framesSinceMoved;
        uint32_t randomSeed;
        uint32_t currentMode;
        uint32_t currentTile;
        uint32_t tileSize;
        uint32_t tileCount;
    };

    void UpdateCameraUniforms(Camera* camera);
    void UpdateMaterialUniforms(const Scene::SceneMaterials& materialInfo);

    void OnKeyPressed(int key) override;
    void OnKeyReleased(int key) override;
    void OnMouseMoved(glm::vec2 newPos) override;
    void OnLeftMouseButtonPressed() override;
    void OnLeftMouseButtonReleased() override;
    void OnRightMouseButtonPressed() override;
    void OnRightMouseButtonReleased() override;

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

    enum class Mode { Realtime, FinalRender };
    Mode mCurrentMode;
    uint32_t mCurrentTile;

    static constexpr uint32_t TileCount = 1440;
};

}  // namespace VKRT