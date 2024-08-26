#pragma once
#include <string>

#include "include/CRTTypes.h"

class TraceHit;
class Ray;
class AABB {
public:
    AABB() = default;

    AABB(const Vec3& min, const Vec3& max) : bounds{ min, max } {}

    /* @brief Create an AABB that encloses all AABBs in the list */
    static AABB MakeEnclosingAABB(std::vector<AABB> aabbs);

    void intersect(const Ray& r, TraceHit& out) const;
    /* @brief Check if this AABB has an intersection with another AABB */
    bool hasIntersection(const AABB& other) const;

    /* @brief Check if this AABB has an intersection with a ray */
    bool hasIntersection(const Ray& ray) const;

    /* @brief Get the distance to the nearest intersection with the AABB on the given axis */
    float distanceToAxis(size_t axis, const Vec3& point) const;

    /* @brief Get the axis with the maximum component
       @return 0: x Axis, 1: y Axis, 2: z Axis */
    size_t getMaxAxis() const;

    /* @brief box volume */
    float volume() const;

    /* @brief Check if the AABB contains a point */
    bool contains(const Vec3& point) const;

    /* @brief Expand the AABB to include the point */
    void expand(const Vec3& point);

    std::string toString() const;

    Vec3 bounds[2]{ Vec3{0.f, 0.f, 0.f}, Vec3{0.f, 0.f, 0.f} };
};
