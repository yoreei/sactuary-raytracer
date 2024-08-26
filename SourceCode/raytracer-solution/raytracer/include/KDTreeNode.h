#pragma once
#include <vector>
#include <array>
#include <memory>
#include <string>

#include "json.hpp"

#include "include/AABB.h"

class Scene;
class TraceHit;
class Ray;
class KDTreeNode
{
    using ordered_json = nlohmann::ordered_json;
    using json = nlohmann::json;
public:
    KDTreeNode() = default;
    KDTreeNode(const AABB& aabb, size_t childId) : aabb(aabb), childId(childId) {}
    KDTreeNode(KDTreeNode&&) = default;
    KDTreeNode& operator=(KDTreeNode&&) = default;
    ~KDTreeNode() = default;

    void build(std::vector<size_t>&& triangleRefs, const std::vector<AABB>& cacheTriangleAABBs, size_t maxTrianglesPerLeaf, size_t maxDepth, size_t depth = 0);
    /* @brief intersect the KDTree with a ray. Write output to `out` */
    void traverse(const Scene& scene, const Ray& ray, TraceHit& out) const;
    json toJson() const;
    std::string toString() const;

    std::array<std::unique_ptr<KDTreeNode>, 2> child {nullptr, nullptr};
    AABB aabb{};
private:
    bool isLeaf() const { return child[0] == nullptr; }
    void traverseRecursive(const Scene& scene, const Ray& ray, TraceHit& out) const;
    /* @return sorted array of children closest to the point */
    std::array<const KDTreeNode*, 2> closestChildren(const Vec3& point) const;
    void intersectMyTriangles(const Scene& scene, const Ray& ray, TraceHit& out) const;

    std::vector<size_t> triangleRefs {};
    /* 0: x Axis, 1: y Axis, 2: z Axis */
    int axisSplit = -1;
    /* if we are a child, which child are we? */
    size_t childId = 0;
};

