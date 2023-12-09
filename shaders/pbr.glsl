float fresnel(const vec3 I, const vec3 N, const float ior) {
    float cosIncident = dot(I, N);
    float etaIncident;
    float etaTrans;
    if (cosIncident > 0.0f) {
        etaIncident = ior;
        etaTrans = 1.0f;
    } else {
        etaIncident = 1.0f;
        etaTrans = ior;
    }
    float sinTrans = etaIncident / etaTrans * sqrt(max(0.f, 1 - cosIncident * cosIncident));
    if (sinTrans >= 1.0f) {
        return 1.0f;
    } else {
        const float cosTrans = sqrt(max(0.f, 1.0f - sinTrans * sinTrans));
        cosIncident = abs(cosIncident);
        float rS = ((etaTrans * cosIncident) - (etaIncident * cosTrans)) /
                   ((etaTrans * cosIncident) + (etaIncident * cosTrans));
        float rP = ((etaIncident * cosIncident) - (etaTrans * cosTrans)) /
                   ((etaIncident * cosIncident) + (etaTrans * cosTrans));
        return clamp((rS * rS + rP * rP) / 2.0f, 0.0f, 1.0f);
    }
}