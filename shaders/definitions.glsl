#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable

const int ColorPayloadIndex = 0;
const int ShadowPayloadIndex = 1;

const int ColorMissIndex = 0;

const float TMin = 0.001;
const float TMax = 1000.0;
const float Infinity = TMax * 100.0;
const uint DefaultSBTOffset = 0;
const uint DefaultSBTStride = 0;

const vec3 AmbientTerm = vec3(0.01);

const float Bias = 0.01;

const float Pi = 3.14159265359;

const uint MaxRecursionLevel = 4;

const uint MaxBounces = 5;

const uint RaysPerPixel = 10;

const uint MaxUInt = 0xFFFFFFFF;

const uint OpaqueMask = 0xF0;
const uint RefractiveMask = 0x0F;
const uint AllMask = OpaqueMask | RefractiveMask;

struct MeshDescription {
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
};

struct Vertex {
    vec3 position;
    vec3 normal;
    vec2 texCoord;
};

struct Material {
    vec3 albedo;
    vec3 emissive;
    float roughness;
    float metallic;
    float indexOfRefraction;
    int albedoTextureIndex;
    int roughnessTextureIndex;
};

struct RayPayload {
    vec3 hitPointPosition;
    vec3 hitPointNormal;
    vec3 hitPointColor;
    vec3 hitPointLight;
    int depth;
};