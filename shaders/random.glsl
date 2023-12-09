uint random(inout uint x) {
    x += (x << 10u);
    x ^= (x >> 6u);
    x += (x << 3u);
    x ^= (x >> 11u);
    x += (x << 15u);
    return x;
}

float random01(inout uint seed) {
    return random(seed) / float(MaxUInt);
}

// https://github.com/ConfettiFX/The-Forge/blob/master/Examples_3/Unit_Tests/src/16_Raytracing/Shaders/Vulkan/ClosestHit.rchit#L86
vec3 sampleCosineWeightedHemisphere(vec2 u) {
    float phi = 2.0f * Pi * u.x;

    float sin_phi = sin(phi);
    float cos_phi = cos(phi);

    float cos_theta = sqrt(u.y);
    float sin_theta = sqrt(1.0f - cos_theta * cos_theta);

    return vec3(sin_theta * cos_phi, cos_theta, sin_theta * sin_phi);
}

// Aligns a direction on the unit hemisphere such that the hemisphere's "up" direction
// (0, 1, 0) maps to the given surface normal direction
vec3 alignHemisphereWithNormal(vec3 localVec, vec3 normal) {
    // Set the "up" vector to the normal
    vec3 up = normal;

    // Find an arbitrary direction perpendicular to the normal. This will become the
    // "right" vector.
    vec3 right = normalize(cross(normal, vec3(0.0072f, 1.0f, 0.0034f)));

    // Find a third vector perpendicular to the previous two. This will be the
    // "forward" vector.
    vec3 forward = cross(right, up);

    // Map the direction on the unit hemisphere to the coordinate system aligned
    // with the normal.
    return localVec.x * right + localVec.y * up + localVec.z * forward;
}

//====
// https://github.com/playdeadgames/temporal/blob/master/Assets/Shaders/IncNoise.cginc
// The MIT License (MIT)
//
// Copyright (c) [2015] [Playdead]
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
// note: normalized random, float=[0, 1]
float PDnrand(vec2 n) {
    return fract(sin(dot(n.xy, vec2(12.9898, 78.233f))) * 43758.5453);
}
vec2 PDnrand2(vec2 n) {
    return fract(sin(dot(n.xy, vec2(12.9898, 78.233f))) * vec2(43758.5453, 28001.8384));
}
vec3 PDnrand3(vec2 n) {
    return fract(sin(dot(n.xy, vec2(12.9898, 78.233f))) * vec3(43758.5453, 28001.8384, 50849.4141));
}
vec4 PDnrand4(vec2 n) {
    return fract(
        sin(dot(n.xy, vec2(12.9898, 78.233f))) *
        vec4(43758.5453, 28001.8384, 50849.4141, 12996.89));
}

// Convert uniform distribution into triangle-shaped distribution.
// https://www.shadertoy.com/view/4t2SDh
// Input is in range [0, 1]
// Output is in range [-1, 1], which is useful for dithering.
vec2 uniformNoiseToTriangular(vec2 n) {
    vec2 orig = n * 2.0 - 1.0;
    n = orig / sqrt(abs(orig));
    n = max(vec2(-1.0), n);
    n = n - vec2(sign(orig));
    return n;
}

vec3 sampleInCosineWeighedHemisphere(vec3 normal, vec2 uv, float seed) {
    vec2 sampleUV = uniformNoiseToTriangular(PDnrand2(uv + seed)) * 0.5 + 0.5;
    vec3 sampleDirLocal = sampleCosineWeightedHemisphere(sampleUV);
    return alignHemisphereWithNormal(sampleDirLocal, normal);
}