#pragma once
#include <string>
#include "CRTTypes.h"

enum class TraceHitType {
    // planeFacingRay = true:
    SUCCESS,                    // rayIntersectsPlnae = true, withinTriangle = true
    OUT_OF_BOUNDS,              // rayIntersectsPlane = true, withinTriangle = false
    PLANE_BEHIND_RAY_ORIGIN,    // rayIntersectsPlane = false, withinTriangle = [true|false]
    // planeFacingRay = false:
    INSIDE_REFRACTIVE,          // rayIntersectsPlane = true, withinTriangle = true
    PLANE_BACKFACE,             // rayIntersectsPlane = [true|false], withinTriangle = [true|false]
    // planeFacingRay = PARALLEL
    PARALLEL,                   // rayIntersectsPlane = false, withinTriangle = false
};

std::string toString(TraceHitType hitType);

class TraceHit {
public:
    float t = FLT_MAX;    // Distance
    Vec3 p = { 0, 0, 0 };     // TraceHit Point
    Vec3 n = { 0, 0, 0 };     // Normal at Point
    float u = 0.f;    // First Barycentric Base
    float v = 0.f;    // Second Barycentric Base
    size_t materialIndex = 0;
    TraceHitType type = TraceHitType::OUT_OF_BOUNDS;
    bool successful() const { return type == TraceHitType::SUCCESS || type == TraceHitType::INSIDE_REFRACTIVE; }
    Vec3 biasP(float bias) const { return p + n * bias; }
};
