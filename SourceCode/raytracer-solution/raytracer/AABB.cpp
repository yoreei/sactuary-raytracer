#include "include/AABB.h"

#include <iostream>
#include <algorithm>
#include <string>
#include <sstream>

#include "include/TraceHit.h"
#include "include/Triangle.h"
#include "include/Scene.h"
#include "include/Metrics.h"
#include "include/TraceHit.h"
#include "include/CRTTypes.h"

AABB AABB::MakeEnclosingAABB(std::vector<AABB> aabbs)
{
    Vec3 min = Vec3::MakeMax();
    Vec3 max = Vec3::MakeLowest();

    for (const auto& aabb : aabbs)
    {
        min.x = std::min(min.x, aabb.bounds[0].x);
        min.y = std::min(min.y, aabb.bounds[0].y);
        min.z = std::min(min.z, aabb.bounds[0].z);

        max.x = std::max(max.x, aabb.bounds[1].x);
        max.y = std::max(max.y, aabb.bounds[1].y);
        max.z = std::max(max.z, aabb.bounds[1].z);
    }

    return { min, max };
}

bool AABB::hasIntersection(const AABB& other) const {
    return (bounds[0].x <= other.bounds[1].x && bounds[1].x >= other.bounds[0].x) &&
        (bounds[0].y <= other.bounds[1].y && bounds[1].y >= other.bounds[0].y) &&
        (bounds[0].z <= other.bounds[1].z && bounds[1].z >= other.bounds[0].z);
}

bool AABB::hasIntersection(const Ray& r) const {
#ifndef NDEBUG
    if (r.getDirection().x != 0.f) assertFEqual(r.invdir.x, 1 / r.getDirection().x);
    if (r.getDirection().y != 0.f) assertFEqual(r.invdir.y, 1 / r.getDirection().y);
    if (r.getDirection().z != 0.f) assertFEqual(r.invdir.z, 1 / r.getDirection().z);
#endif

    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    tmin = (bounds[r.sign[0]].x - r.origin.x) * r.invdir.x;
    tmax = (bounds[1 - r.sign[0]].x - r.origin.x) * r.invdir.x;
    tymin = (bounds[r.sign[1]].y - r.origin.y) * r.invdir.y;
    tymax = (bounds[1 - r.sign[1]].y - r.origin.y) * r.invdir.y;

    if ((tmin > tymax) || (tymin > tmax))
        return false;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    tzmin = (bounds[r.sign[2]].z - r.origin.z) * r.invdir.z;
    tzmax = (bounds[1 - r.sign[2]].z - r.origin.z) * r.invdir.z;


    if ((tmin > tzmax) || (tzmin > tmax))
        return false;

    //if (tzmin > tmin)
    //    tmin = tzmin;
    //if (tzmax < tmax)
    //    tmax = tzmax;

    return true;
}

void AABB::intersect(const Ray& r, TraceHit& out) const
{
#ifndef NDEBUG
    if (r.getDirection().x != 0.f) assertFEqual(r.invdir.x, 1 / r.getDirection().x);
    if (r.getDirection().y != 0.f) assertFEqual(r.invdir.y, 1 / r.getDirection().y);
    if (r.getDirection().z != 0.f) assertFEqual(r.invdir.z, 1 / r.getDirection().z);
#endif

	out.type = TraceHitType::OUT_OF_BOUNDS;
    float tmin, tmax, tymin, tymax, tzmin, tzmax;

    tmin = (bounds[r.sign[0]].x - r.origin.x) * r.invdir.x;
    tmax = (bounds[1 - r.sign[0]].x - r.origin.x) * r.invdir.x;
    tymin = (bounds[r.sign[1]].y - r.origin.y) * r.invdir.y;
    tymax = (bounds[1 - r.sign[1]].y - r.origin.y) * r.invdir.y;

    if ((tmin > tymax) || (tymin > tmax))
        return;

    if (tymin > tmin)
        tmin = tymin;
    if (tymax < tmax)
        tmax = tymax;

    tzmin = (bounds[r.sign[2]].z - r.origin.z) * r.invdir.z;
    tzmax = (bounds[1 - r.sign[2]].z - r.origin.z) * r.invdir.z;


    if ((tmin > tzmax) || (tzmin > tmax))
        return;

    if (tzmin > tmin)
        tmin = tzmin;
    if (tzmax < tmax)
        tmax = tzmax;

    if (tmin >= 0) {
        out.t = tmin;
        out.type = TraceHitType::SUCCESS;
    }
    else if (tmax >= 0) {
        out.t = tmax;
        out.type = TraceHitType::SUCCESS;
    }
}


float AABB::distanceToAxis(size_t axis, const Vec3& point) const
{
    float distanceBounds0 = std::fabs(bounds[0].axis(axis) - point.axis(axis));
    float distanceBounds1 = std::fabs(bounds[1].axis(axis) - point.axis(axis));
    float distance = std::min(distanceBounds0, distanceBounds1);

    return distance;
}

size_t AABB::getMaxAxis() const
{
    Vec3 extents = bounds[1] - bounds[0];
    if (extents.x > extents.y && extents.x > extents.z) {
        return 0; // x-axis
    }
    else if (extents.y > extents.z) {
        return 1; // y-axis
    }
    else {
        return 2; // z-axis
    }
}

bool AABB::contains(const Vec3& point) const {
    return (point.x >= bounds[0].x - epsilon && point.x <= bounds[1].x + epsilon) &&
        (point.y >= bounds[0].y - epsilon && point.y <= bounds[1].y + epsilon) &&
        (point.z >= bounds[0].z - epsilon && point.z <= bounds[1].z + epsilon);
}

void AABB::expand(const Vec3& point)
{
    bounds[0].x = std::min(bounds[0].x, point.x);
    bounds[0].y = std::min(bounds[0].y, point.y);
    bounds[0].z = std::min(bounds[0].z, point.z);

    bounds[1].x = std::max(bounds[1].x, point.x);
    bounds[1].y = std::max(bounds[1].y, point.y);
    bounds[1].z = std::max(bounds[1].z, point.z);
}

inline std::string AABB::toString() const {
    std::stringstream ss;
    ss << "bounds[0]: (" << bounds[0].x << ", " << bounds[0].y << ", " << bounds[0].z << ")\n";
    ss << "bounds[1]: (" << bounds[1].x << ", " << bounds[1].y << ", " << bounds[1].z << ")\n";
    return ss.str();
}

float AABB::volume() const {
    float width = bounds[1].x - bounds[0].x;
    float height = bounds[1].y - bounds[0].y;
    float depth = bounds[1].z - bounds[0].z;

    return width * height * depth;
}
