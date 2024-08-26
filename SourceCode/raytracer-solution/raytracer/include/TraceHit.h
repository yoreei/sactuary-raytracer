#pragma once
#include <string>

#include "include/CRTTypes.h"

enum class TraceHitType {
    // planeFacingRay = true:
    SUCCESS,                    // rayIntersectsPlnae = true, withinTriangle = true
    OUT_OF_BOUNDS,              // rayIntersectsPlane = true, withinTriangle = false
    PLANE_BEHIND_RAY_ORIGIN,    // rayIntersectsPlane = false, withinTriangle = [true|false]
    // planeFacingRay = false:
    INSIDE_REFRACTIVE,          // rayIntersectsPlane = true, withinTriangle = true
    PLANE_BACKFACE,             // rayIntersectsPlane = [true|false], withinTriangle = [true|false]
    SMOOTH_SHADING_BACKFACE,
    // planeFacingRay = PARALLEL
    PARALLEL,                   // rayIntersectsPlane = false, withinTriangle = false
    SMOOTH_SHADING_PARALLEL,
    // OTHER
    AABB_PRUNE,

};

std::string toString(TraceHitType hitType);

class TraceHit {
public:
    float t = FLT_MAX;    // Distance
    Vec3 p = { 0, 0, 0 };     // Intersection Point
    Vec3 n = { 0, 0, 0 };     // Normal at Point
    float baryU = 0.f;  // Barycentric U coordinate
    float baryV = 0.f;  // Barycentric V coordinate
    float u = 0.f;    // Texture U coordinate
    float v = 0.f;    // Texture V coordinate
    size_t materialIndex = 0; // see Scene::materials
    TraceHitType type = TraceHitType::OUT_OF_BOUNDS;
    size_t triRef = 0;
    size_t kdtreeIdx = 0;
    bool successful() const { return type == TraceHitType::SUCCESS || type == TraceHitType::INSIDE_REFRACTIVE; }
    /* @brief Get point of intersection adjusted for bias */
    Vec3 biasP(float bias) const { return p + n * bias; }
};

