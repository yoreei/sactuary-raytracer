#include "include/KDTreeNode.h"

#include "json.hpp"

#include "include/TraceHit.h"
#include "include/CRTTypes.h"
#include "include/Scene.h"

using ordered_json = nlohmann::ordered_json;

void KDTreeNode::traverse(const Scene& scene, const Ray& ray, TraceHit& out) const {
	out.t = std::numeric_limits<float>::max();
	out.type = TraceHitType::OUT_OF_BOUNDS;

	traverseRecursive(scene, ray, out);
}

json KDTreeNode::toJson() const
{
	json j{};
	j["min"] = { aabb.bounds[0].x, aabb.bounds[0].y, aabb.bounds[0].z };
	j["max"] = { aabb.bounds[1].x, aabb.bounds[1].y, aabb.bounds[1].z };
	j["triangleRefs"] = triangleRefs;

	if (child[0]) j["child0"] = child[0]->toJson();
	if (child[1]) j["child1"] = child[1]->toJson();
	return j;
}

// TODO optimize triangle intersections by sorting by ray - plane intersection distance first
void KDTreeNode::traverseRecursive(const Scene& scene, const Ray& ray, TraceHit& out) const {
#ifndef NDEBUG
	std::cout << "traversing [child: " << childId << "], [axisSplit: " << axisSplit << "]\n";
#endif

	if (!aabb.hasIntersection(ray)) {
		out.type = TraceHitType::OUT_OF_BOUNDS;
		return;
	}

	if (isLeaf()) {
		intersectMyTriangles(scene, ray, out);
		//aabb.intersect(ray, out);
		//if (out.successful()) {
		//	out.kdtreeIdx = childId;
		//}
		return;
	}
	else {
		std::array<const KDTreeNode*, 2> sortedChildren = closestChildren(ray.origin);

		for (size_t i = 0; i < sortedChildren.size(); ++i) {
			const KDTreeNode* childPtr = sortedChildren[i];
			childPtr->traverseRecursive(scene, ray, out);

			if (out.successful()) {
				// std::cout << "success in childId: " << childPtr->childId << "\n";
				out.kdtreeIdx = i;
				break;
			}
		}
	}
}

void KDTreeNode::intersectMyTriangles(const Scene& scene, const Ray& ray, TraceHit& out) const {
	for (const size_t& triRef : triangleRefs) {
		const Triangle& tri = scene.triangles[triRef];

		TraceHit tryHit{};
		tri.intersect(scene, ray, triRef, tryHit);
		if (tryHit.successful() && tryHit.t < out.t && aabb.contains(tryHit.p)) {
			out = tryHit;
		}
	}
}

//std::array<const KDTreeNode*, 2> KDTreeNode::closestChildren(const Vec3& point) const {
//	assert(axisSplit >= 0 && axisSplit <= 2);
//
//	if (child[0]->aabb.contains(point)) {
//		return { child[0].get(), child[1].get() };
//	}
//	if (child[1]->aabb.contains(point)) {
//		return { child[1].get(), child[0].get() };
//	}
//
//	float dist0 = child[0]->aabb.distanceToAxis(axisSplit, point);
//	float dist1 = child[1]->aabb.distanceToAxis(axisSplit, point);
//	if (dist0 < dist1) {
//		return { child[0].get(), child[1].get() };
//	}
//	else {
//		return { child[1].get(), child[0].get() };
//	}
//}

std::array<const KDTreeNode*, 2> KDTreeNode::closestChildren(const Vec3& point) const {
    assert(axisSplit >= 0 && axisSplit <= 2);

    const KDTreeNode* firstChild = child[0].get();
    const KDTreeNode* secondChild = child[1].get();

	zzz precalculate this for all initial rays
    if (firstChild->aabb.contains(point)) {
        return { firstChild, secondChild };
    } 
    if (secondChild->aabb.contains(point)) {
        return { secondChild, firstChild };
    }

    float dist0 = std::fabs(firstChild->aabb.bounds[0].axis(axisSplit) - point.axis(axisSplit));
    float dist1 = std::fabs(secondChild->aabb.bounds[0].axis(axisSplit) - point.axis(axisSplit));
    dist0 = std::min(dist0, std::fabs(firstChild->aabb.bounds[1].axis(axisSplit) - point.axis(axisSplit)));
    dist1 = std::min(dist1, std::fabs(secondChild->aabb.bounds[1].axis(axisSplit) - point.axis(axisSplit)));

    if (dist0 < dist1) {
        return { firstChild, secondChild };
    } else {
        return { secondChild, firstChild };
    }
}

void KDTreeNode::build(std::vector<size_t>&& newTriangleRefs, const std::vector<AABB>& cacheTriangleAABBs, size_t maxTrianglesPerLeaf, size_t maxDepth, size_t depth)
{
	// Required: newTriangleRefs intersect aabb

	// 0.1 Recursion Root (Make Leaf)
	if (depth >= maxDepth || newTriangleRefs.size() <= maxTrianglesPerLeaf) {
		triangleRefs = std::move(newTriangleRefs);
		return; // Leaf
	}

	// 0.2 Recursion Root 2. Stop if AABB too small. Check only X axis for performance reasons.
	float size = aabb.bounds[1].x - aabb.bounds[0].x;
	if (size <= 1.0E-4F) {
		triangleRefs = std::move(newTriangleRefs);
		return; // Leaf
	}

	// 1. Split Triangles in 2 Groups (Potential Children Nodes)
	// 1.1 Axis Splitting And Compute axisSplit
	axisSplit = int(aabb.getMaxAxis());
	float splitValue = (aabb.bounds[0].axis(axisSplit) + aabb.bounds[1].axis(axisSplit)) / 2;

	std::vector<size_t> triangleRefs0{};
	AABB child0Aabb = aabb;
	child0Aabb.bounds[1].axis(axisSplit) = splitValue;

	std::vector<size_t> triangleRefs1{};
	AABB child1Aabb = aabb;
	child1Aabb.bounds[0].axis(axisSplit) = splitValue;

	for (const auto& candidateRef : newTriangleRefs) {
		int dbgPushBacks = 0;
		if (child0Aabb.hasIntersection(cacheTriangleAABBs[candidateRef])) {
			triangleRefs0.push_back(candidateRef);
			++dbgPushBacks;
		}
		if (child1Aabb.hasIntersection(cacheTriangleAABBs[candidateRef])) {
			triangleRefs1.push_back(candidateRef);
			++dbgPushBacks;
		}
		assert(dbgPushBacks > 0);
	}
	assert(triangleRefs0.size() + triangleRefs1.size() >= newTriangleRefs.size());

	// 3. Else, Create Children
	child[0] = std::make_unique<KDTreeNode>(child0Aabb, 0);
	child[1] = std::make_unique<KDTreeNode>(child1Aabb, 1);
	child[0]->build(std::move(triangleRefs0), cacheTriangleAABBs, maxTrianglesPerLeaf, maxDepth, depth + 1);
	child[1]->build(std::move(triangleRefs1), cacheTriangleAABBs, maxTrianglesPerLeaf, maxDepth, depth + 1);
}

std::string KDTreeNode::toString() const
{
	return (*this).toJson().dump(4);
}
