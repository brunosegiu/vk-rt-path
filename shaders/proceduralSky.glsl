// From: https://github.com/nvpro-samples/nvpro_core/blob/master/nvvkhl/shaders/dh_sky.h
struct ProceduralSkyShaderParameters {
    vec3 directionToLight;
    float angularSizeOfLight;
    vec3 lightColor;
    float glowSize;
    vec3 skyColor;
    float glowIntensity;
    vec3 horizonColor;
    float horizonSize;
    vec3 groundColor;
    float glowSharpness;
    vec3 directionUp;
    float pad1;
};

ProceduralSkyShaderParameters initSkyShaderParameters(vec3 directionToLight) {
    ProceduralSkyShaderParameters parameters;
    parameters.directionToLight = directionToLight;
    parameters.angularSizeOfLight = 0.059;
    parameters.lightColor = vec3(1.0, 1.0, 1.0);
    parameters.skyColor = vec3(0.17, 0.37, 0.65);
    parameters.horizonColor = vec3(0.50, 0.70, 0.92);
    parameters.groundColor = vec3(0.62, 0.59, 0.55);
    parameters.directionUp = vec3(0., 1., 0.);
    parameters.horizonSize = 0.5;
    parameters.glowSize = 0.091;
    parameters.glowIntensity = 0.9;
    parameters.glowSharpness = 4.;

    return parameters;
}

vec3 getProceduralSkyColor(
    ProceduralSkyShaderParameters params,
    vec3 direction,
    float angularSizeOfPixel
) {
    float elevation = asin(clamp(dot(direction, params.directionUp), -1.0, 1.0));
    float top = smoothstep(0., params.horizonSize, elevation);
    float bottom = smoothstep(0., params.horizonSize, -elevation);
    vec3 environment = mix(mix(params.horizonColor, params.groundColor, bottom), params.skyColor, top);

    float angleToLight = acos(clamp(dot(direction, params.directionToLight), 0.0, 1.0));
    float halfAngularSize = params.angularSizeOfLight * 0.5;
    float lightIntensity = clamp(1.0 - smoothstep(halfAngularSize - angularSizeOfPixel * 2.0, halfAngularSize + angularSizeOfPixel * 2.0, angleToLight), 0.0, 1.0);
    lightIntensity = pow(lightIntensity, 4.0);
    float glow_input = clamp(2.0 * (1.0 - smoothstep(halfAngularSize - params.glowSize, halfAngularSize + params.glowSize, angleToLight)), 0.0, 1.0);
    float glow_intensity = params.glowIntensity * pow(glow_input, params.glowSharpness);
    vec3 light = max(lightIntensity, glow_intensity) * params.lightColor;

    return environment + light;
}
