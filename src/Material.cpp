#include "Material.h"

#include "DebugUtils.h"
#include "Device.h"

namespace VKRT {

Material::Material() : Material(glm::vec3(0.5), glm::vec3(0.0), 1.0f, 0.0f, 0.0f, 1.0f) {}

Material::Material(
    const glm::vec3& albedo,
    const glm::vec3& emissive,
    float roughness,
    float metallic,
    float transmission,
    float indexOfRefraction,
    ScopedRefPtr<Texture> albedoTexture,
    ScopedRefPtr<Texture> roughnessTexture)
    : mAlbedo(albedo),
      mEmissive(emissive),
      mRoughness(roughness),
      mMetallic(metallic),
      mTransmission(transmission),
      mIndexOfRefraction(indexOfRefraction),
      mAlbedoTexture(albedoTexture),
      mRoughnessTexture(roughnessTexture) {}

Material::~Material() {}

}  // namespace VKRT