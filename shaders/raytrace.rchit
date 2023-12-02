#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "definitions.glsl"

layout(location = ColorPayloadIndex) rayPayloadInEXT RayPayload rayPayload;
layout(location = ShadowPayloadIndex) rayPayloadEXT float shadowAttenuation;
hitAttributeEXT vec2 hitAttributes;

layout(binding = 0, set = 0) uniform accelerationStructureEXT topLevelAS;
layout(buffer_reference, scalar) buffer Vertices {
    Vertex values[];
};
layout(buffer_reference, scalar) buffer Indices {
    uvec3 values[];
};
layout(binding = 3, set = 0, scalar) buffer Description_ {
    MeshDescription values[];
}
descriptions;
layout(binding = 4, set = 0) uniform sampler textureSampler;
layout(binding = 5, set = 0, scalar) buffer Material_ {
    Material values[];
}
materials;
layout(binding = 6, set = 0) uniform texture2D sceneTextures[];

Vertex unpackInstanceVertex(const int intanceId) {
    MeshDescription description = descriptions.values[intanceId];
    Indices indices = Indices(description.indexBufferAddress);
    Vertices vertices = Vertices(description.vertexBufferAddress);

    uvec3 triangleIndices = indices.values[gl_PrimitiveID];
    Vertex v0 = vertices.values[triangleIndices.x];
    Vertex v1 = vertices.values[triangleIndices.y];
    Vertex v2 = vertices.values[triangleIndices.z];

    const vec3 barycentricCoords =
        vec3(1.0 - hitAttributes.x - hitAttributes.y, hitAttributes.x, hitAttributes.y);

    const vec3 position = v0.position * barycentricCoords.x + v1.position * barycentricCoords.y +
                          v2.position * barycentricCoords.z;
    const vec3 worldSpacePosition = vec3(gl_ObjectToWorldEXT * vec4(position, 1.0));

    const vec3 normal = v0.normal * barycentricCoords.x + v1.normal * barycentricCoords.y +
                        v2.normal * barycentricCoords.z;
    const vec3 worldSpaceNormal = normalize(vec3(normal * gl_WorldToObjectEXT));

    const vec2 texCoord = v0.texCoord * barycentricCoords.x + v1.texCoord * barycentricCoords.y +
                          v2.texCoord * barycentricCoords.z;

    return Vertex(worldSpacePosition, worldSpaceNormal, texCoord);
}

Material unpackInstanceMaterial(const int intanceId) {
    return materials.values[intanceId];
}

vec3 getAlbedo(const Material material, const vec2 texCoord) {
    vec3 albedo = material.albedo.rgb;
    if (material.albedoTextureIndex >= 0) {
        albedo =
            texture(sampler2D(sceneTextures[material.albedoTextureIndex], textureSampler), texCoord)
                .rgb;
    }
    return albedo;
}

void getRoughnessAndMetallic(
    const Material material,
    const vec2 texCoord,
    out float roughness,
    out float metallic) {
    roughness = material.roughness;
    metallic = material.metallic;
    if (material.roughnessTextureIndex >= 0) {
        vec4 textureSample = texture(
            sampler2D(sceneTextures[material.roughnessTextureIndex], textureSampler),
            texCoord);
        metallic = textureSample.b;
        roughness = textureSample.g;
    }
}

void main() {
    if (rayPayload.depth > MaxRecursionLevel) {
        rayPayload.depth = -1;
        return;
    }

    rayPayload.depth += 1;

    const Vertex vertex = unpackInstanceVertex(gl_InstanceCustomIndexEXT);
    const Material material = unpackInstanceMaterial(gl_InstanceCustomIndexEXT);

    const vec3 albedo = getAlbedo(material, vertex.texCoord);

    rayPayload.hitPointPosition = vertex.position;
    rayPayload.hitPointNormal = vertex.normal;
    rayPayload.hitPointColor = albedo;
    rayPayload.hitPointLight = material.emissive;
}
