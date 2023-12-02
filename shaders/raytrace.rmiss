#version 460
#extension GL_EXT_ray_tracing : enable
#extension GL_EXT_buffer_reference : enable
#extension GL_EXT_scalar_block_layout : enable
#extension GL_GOOGLE_include_directive : enable

#include "definitions.glsl"

layout(location = ColorPayloadIndex) rayPayloadInEXT RayPayload rayPayload;

void main() {
    rayPayload.depth = -1;
}