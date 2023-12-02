#include <chrono>

#include "Camera.h"
#include "Context.h"
#include "DebugUtils.h"
#include "Renderer.h"
#include "Scene.h"
#include "Window.h"

struct Timer {
    void Start() { beginTime = std::chrono::steady_clock::now(); }
    template <typename M>
    uint64_t Elapsed() {
        return std::chrono::duration_cast<M>(std::chrono::steady_clock::now() - beginTime).count();
    }
    uint64_t ElapsedMillis() { return Elapsed<std::chrono::milliseconds>(); }
    uint64_t ElapsedMicros() { return Elapsed<std::chrono::microseconds>(); }
    double ElapsedSeconds() {
        return static_cast<double>(Elapsed<std::chrono::microseconds>()) / 1000000.0;
    }

    std::chrono::steady_clock::time_point beginTime;
};

int main() {
    using namespace VKRT;
    auto [windowResult, window] = Window::Create();
    VKRT_ASSERT_MSG(windowResult == Result::Success, "Couldn't create window");
    if (windowResult == Result::Success) {
        auto [contextResult, context] = window->CreateContext();
        VKRT_ASSERT_MSG(contextResult == Result::Success, "No compatible GPU found");
        if (contextResult == Result::Success) {
            ScopedRefPtr<Scene> scene = new Scene(context);

            std::vector<ScopedRefPtr<Model>> cubes{
                Model::Load(context, "./assets/sphere.glb"),
                Model::Load(context, "./assets/sphere.glb"),
                Model::Load(context, "./assets/sphere.glb"),
            };

            std::vector<ScopedRefPtr<Model>> spheres{
                Model::Load(context, "./assets/sphere.glb"),
                Model::Load(context, "./assets/sphere.glb"),
                Model::Load(context, "./assets/sphere.glb"),
            };

            ScopedRefPtr<Model> sphere = Model::Load(context, "./assets/cube.glb");
            std::for_each(sphere->GetMeshes().begin(), sphere->GetMeshes().end(), [](Mesh* mesh) {
                mesh->GetMaterial()->SetEmissive(glm::vec3(1.0f));
            });

            ScopedRefPtr<Model> ground = Model::Load(context, "./assets/box.glb");
            std::for_each(ground->GetMeshes().begin(), ground->GetMeshes().end(), [](Mesh* mesh) {
                mesh->GetMaterial()->SetAlbedo(glm::vec3(0.5f));
            });

            std::for_each(
                cubes[0]->GetMeshes().begin(),
                cubes[0]->GetMeshes().end(),
                [](Mesh* mesh) {
                    mesh->GetMaterial()->SetAlbedo(glm::vec3(0.0f, 0.0f, 1.0f));
                    mesh->GetMaterial()->SetRoughness(0.5f);
                });

            std::for_each(
                cubes[1]->GetMeshes().begin(),
                cubes[1]->GetMeshes().end(),
                [](Mesh* mesh) {
                    mesh->GetMaterial()->SetMetallic(0.8f);
                    mesh->GetMaterial()->SetRoughness(0.0f);
                });

            std::for_each(
                cubes[2]->GetMeshes().begin(),
                cubes[2]->GetMeshes().end(),
                [](Mesh* mesh) { mesh->GetMaterial()->SetIndexOfRefraction(1.3f); });

            std::for_each(
                spheres[0]->GetMeshes().begin(),
                spheres[0]->GetMeshes().end(),
                [](Mesh* mesh) {
                    mesh->GetMaterial()->SetAlbedo(glm::vec3(0.2f, 0.6f, 0.3f));
                    mesh->GetMaterial()->SetRoughness(0.8f);
                });

            std::for_each(
                spheres[1]->GetMeshes().begin(),
                spheres[1]->GetMeshes().end(),
                [](Mesh* mesh) { mesh->GetMaterial()->SetAlbedo(glm::vec3(0.4f, 0.0f, 1.0f)); });

            std::for_each(
                spheres[2]->GetMeshes().begin(),
                spheres[2]->GetMeshes().end(),
                [](Mesh* mesh) { mesh->GetMaterial()->SetIndexOfRefraction(1.8f); });

            ScopedRefPtr<Camera> camera = new Camera(window);
            camera->SetTranslation(glm::vec3(-8.0f, -5.0f, 0.0f));
            camera->SetRotation(glm::vec3(0.0f, -90.0f, 0.0f));

            float offset = 0.0f;
            for (const auto& cube : cubes) {
                ScopedRefPtr<Object> cubeObject = new Object(cube);
                cubeObject->SetTranslation(glm::vec3(0.0f, 2.0f, 3.0f - offset));
                cubeObject->SetScale(glm::vec3(0.75));
                cubeObject->Rotate(glm::vec3(0.0f, offset * 20, 0.0f));
               // scene->AddObject(cubeObject);
                offset += 3.0f;
            }

            offset = 0.0f;
            for (const auto& sphere : spheres) {
                ScopedRefPtr<Object> object = new Object(sphere);
                object->SetTranslation(glm::vec3(1.0f, 6.0f, 3.0f - offset));
                object->SetScale(glm::vec3(0.75));
               // scene->AddObject(object);
                offset += 3.0f;
            }

            ScopedRefPtr<Object> lightObj = new Object(sphere);
            lightObj->SetTranslation(glm::vec3(5.0f, 9.5f, 0.0f));
            lightObj->SetScale(glm::vec3(1.0f, 0.1f, 1.0f));
            scene->AddObject(lightObj);

            ScopedRefPtr<Object> groundObj = new Object(ground);
            groundObj->SetTranslation(glm::vec3(1.0f, 5.0f, 0.0f));
            groundObj->SetScale(glm::vec3(5.0f, 5.0f, 4.0f));
            groundObj->Rotate(glm::vec3(0.0f, 90.0f, 0.0f));
            scene->AddObject(groundObj);

            {
                ScopedRefPtr<Model> dragon = Model::Load(context, "./assets/DragonAttenuation.glb");
                std::for_each(
                    sphere->GetMeshes().begin(),
                    sphere->GetMeshes().end(),
                    [](Mesh* mesh) {  });
                ScopedRefPtr<Object> dragonObj = new Object(dragon);
                dragonObj->SetTranslation(glm::vec3(0.0f, 0.0f, 0.0f));
                dragonObj->SetScale(glm::vec3(5.0f, 7.5f, 3.0f));
                dragonObj->Rotate(glm::vec3(0.0f, 90.0f, 0.0f));
                scene->AddObject(dragonObj);
            }

            {
                ScopedRefPtr<Model> mesh = Model::Load(context, "./assets/venus.gltf");
                std::for_each(
                    sphere->GetMeshes().begin(),
                    sphere->GetMeshes().end(),
                    [](Mesh* mesh) {
                        mesh->GetMaterial()->SetAlbedo(glm::vec3(0.9f, 0.87f, 0.8f));
                    });
                ScopedRefPtr<Object> object = new Object(mesh);
                object->SetTranslation(glm::vec3(2.5f, 3.5f, 0.0f));
                object->SetScale(glm::vec3(0.6f));
                object->Rotate(glm::vec3(90.0f, 90.0f, 0.0f));
                scene->AddObject(object);
            }

            ScopedRefPtr<Renderer> renderer = new Renderer(context, scene);
            Timer timer;
            double elapsedSeconds = 0.0;
            double totalSeconds = 0.0;
            while (window->Update()) {
                timer.Start();
                {
                    camera->Update(elapsedSeconds);
                    renderer->Render(camera);
                }
                elapsedSeconds = timer.ElapsedSeconds();
                totalSeconds += elapsedSeconds;
            }
        }
        if (context != nullptr) {
            window->DestroyContext();
        }
    }
    return 0;
}