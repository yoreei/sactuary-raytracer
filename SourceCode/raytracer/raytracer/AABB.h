#pragma once
#include <string>
#include <array>
#include <memory>

#include "CRTTypes.h"

class TraceHit;
class Scene;
class Ray;
class Triangle;
class AABB {
public:
    AABB() = default;

    AABB(const Vec3& min, const Vec3& max) : bounds{min, max} {}

    bool hasIntersection(const AABB& other) const;

    bool hasIntersection(const Ray& ray) const;

    /* @brief Assumes hasIntersection(ray) == true */
    void traverse(const Scene& scene, const Ray& ray, TraceHit& out) const;

    bool contains(const Vec3& point) const;

    void expandWithTriangle(const Scene& scene, const Triangle& triangle, const size_t triangleRef);

    Vec3 getCenter() const;

    Vec3 getSize() const;

    std::string toString() const;

    Vec3 bounds[2] {Vec3{0.f, 0.f, 0.f}, Vec3{0.f, 0.f, 0.f}};
private:
    std::vector<size_t> triangleRefs {};

    bool isLeaf() const { return child[0] == nullptr; }

    std::array<std::unique_ptr<AABB>, 2> child {nullptr, nullptr};

    void expandWithPoint(const Vec3& point);
};