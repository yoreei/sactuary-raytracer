#pragma warning( disable : 4365 )

#include "json.hpp"

#include "include/Triangle.h"
#include "include/Scene.h"

Triangle::Triangle(size_t v0, size_t v1, size_t v2, size_t _materialIndex)
{
    v[0] = v0;
    v[1] = v1;
    v[2] = v2;
    materialIndex = _materialIndex;
}

void Triangle::buildNormal(const std::vector<Vec3>& vertices) {
    const Vec3& v0 = vertices[v[0]];
    const Vec3& v1 = vertices[v[1]];
    const Vec3& v2 = vertices[v[2]];

    Vec3 e1 = v1 - v0;
    Vec3 e2 = v2 - v0;

    e1.cross(e2, normal);
    normal.normalize();
}

[[nodiscard]]
Vec3 Triangle::getNormal() const {
    return normal;
}

float Triangle::area(const std::vector<Vec3>& vertices) const {
    const Vec3& v0 = vertices[v[0]];
    const Vec3& v1 = vertices[v[1]];
    const Vec3& v2 = vertices[v[2]];

    Vec3 e1 = v1 - v0;
    Vec3 e2 = v2 - v0;
    return e1.crossLength(e2) * 0.5f;
}

std::string Triangle::toString(const std::vector<Vec3>& vertices) const {
    const Vec3& v0 = vertices[v[0]];
    const Vec3& v1 = vertices[v[1]];
    const Vec3& v2 = vertices[v[2]];

    return "Triangle: {" + v0.toString() + ", " + v1.toString() + ", " + v2.toString() + "}";
}

bool Triangle::hasVertex(size_t vertexIndex) const {
    return v[0] == vertexIndex || v[1] == vertexIndex || v[2] == vertexIndex;
}

void Triangle::intersect(const Scene& scene, const Ray& ray, size_t triRef, TraceHit& hit) const {
    assertFEqual(ray.getDirection().lengthSquared(), 1.f);
    GSceneMetrics.record("TriangleIntersection");
    if (!scene.cacheTriangleAABBs[triRef].hasIntersection(ray)) return;

	hit.triRef = triRef;
    float rayProj = ray.getDirection().dot(normal);

    if (rayProj < -1e-6) { // normal is facing ray
        computeHit(scene, ray, rayProj, hit);
    }
    else if (rayProj > 1e-6) { // normal is facing away from ray
        auto& material = scene.materials[materialIndex];
        if (material.type == Material::Type::REFRACTIVE) {
            Triangle swapT = swappedTriangle();
            swapT.intersect(scene, ray, triRef, hit);
            if (hit.type == TraceHitType::SUCCESS) {
                hit.type = TraceHitType::INSIDE_REFRACTIVE;
                hit.n = -hit.n;
                assert(dot(hit.n, ray.getDirection()) > 1e-6); // ray is exiting refractive material
            }
        }
        else {
            hit.type = TraceHitType::PLANE_BACKFACE;
        }
    }
    else {
        hit.type = TraceHitType::PARALLEL;
    }

}

bool Triangle::fastIntersect(const Scene& scene, const Vec3& start, const Vec3& end) const
{
    const Vec3& v0 = scene.vertices[v[0]];
    const Vec3& v1 = scene.vertices[v[1]];
    const Vec3& v2 = scene.vertices[v[2]];

    if (signOfVolume(start, v0, v1, v2) != signOfVolume(end, v0, v1, v2)) {
        bool pyr3 = signOfVolume(start, end, v0, v1);
        if (pyr3 == signOfVolume(start, end, v1, v2) && pyr3 == signOfVolume(start, end, v2, v0)) {
            return true;
        }
    }
    return false;
}

void Triangle::computeHit(const Scene& scene, const Ray& ray, float rProj, TraceHit& hit) const {
    const Vec3& n = normal;
    const Vec3& v0 = scene.vertices[v[0]];
    const Vec3& v1 = scene.vertices[v[1]];
    const Vec3& v2 = scene.vertices[v[2]];

    // Here assuming counter-clockwise order
    Vec3 e0 = v1 - v0;
    Vec3 e1 = v2 - v0;
    float rpDist = dot(n, v0 - ray.origin); // Ray-Plane distance
    hit.t = rpDist / rProj;
    if (hit.t < 0) {
        hit.type = TraceHitType::PLANE_BEHIND_RAY_ORIGIN;
        return;
    }
    hit.p = ray.origin + ray.getDirection() * hit.t;

    // check if `p` is inside triangle
    Vec3 v0p = hit.p - v0;
    Vec3 e2 = v2 - v1;
    Vec3 c0 = cross(e0, v0p);
    Vec3 c1 = cross(v0p, e1);
    Vec3 c2 = cross(e2, hit.p - v1);

    bool inside = dot(n, c0) > -1e-6 &&
        dot(n, c1) > -1e-6 &&
        dot(n, c2) > -1e-6;

    if (inside) {
        float area_inv = 1.f / e0.crossLength(e1);
        hit.baryU = c1.length() * area_inv;
        hit.baryV = c0.length() * area_inv;
        float baryW = 1.0f - hit.baryU - hit.baryV;
        const Vec3& uvMap0 = scene.uvs[v[0]];
        const Vec3& uvMap1 = scene.uvs[v[1]];
        const Vec3& uvMap2 = scene.uvs[v[2]];

        hit.u = uvMap0.x * baryW + uvMap1.x * hit.baryU + uvMap2.x * hit.baryV;
        hit.v = uvMap0.y * baryW + uvMap1.y * hit.baryU + uvMap2.y * hit.baryV;

        hit.materialIndex = this->materialIndex;
        hit.n = hitNormal(scene, hit);
        hit.type = getTraceHitType(hit.n, ray.getDirection());
        assertHit(scene, hit);
        return;
    }
    else {
        hit.type = TraceHitType::OUT_OF_BOUNDS;
        return;
    }
}

TraceHitType Triangle::getTraceHitType(const Vec3& n, const Vec3& rayDir) {
    float rProj = dot(rayDir, n);
    if (rProj < -epsilon) {
        return TraceHitType::SUCCESS;
    }
    else if (rProj > epsilon) {
        return TraceHitType::SMOOTH_SHADING_BACKFACE;
    }
    else {
        return TraceHitType::SMOOTH_SHADING_PARALLEL;
    }
}

void Triangle::assertHit(const Scene& scene, const TraceHit& hit) {
    // This function should be optimized away by the compiler in release builds
    // Suppress unused parameter warnings for Release:
    scene;
    hit;
#ifndef NDEBUG
    if (hit.type == TraceHitType::SMOOTH_SHADING_BACKFACE || hit.type == TraceHitType::SMOOTH_SHADING_PARALLEL) {
        assert(scene.materials[hit.materialIndex].smoothShading);
    }
    if (hit.successful()) {
        assert(hit.t >= 0);
    }

#endif
}

Triangle Triangle::swappedTriangle() const
{
    Triangle t = *this;
    t.swapFace();
    return t;
}

void Triangle::swapFace() {
    auto v2buff = v[2];
    v[2] = v[1];
    v[1] = v2buff;
    normal = -normal;
}

bool Triangle::intersect_plane(const std::vector<Vec3>& vertices, const Ray& ray, float& t, Vec3& p) const {
    const Vec3& v0 = vertices[v[0]];
    const Vec3& v1 = vertices[v[1]];
    const Vec3& v2 = vertices[v[2]];

    // We use counter-clockwise winding order
    Vec3 e0 = v1 - v0;
    Vec3 e1 = v2 - v0;

    float rayProj = dot(ray.getDirection(), normal);

    // if triangle is facing the ray, compute ray-plane intersection
    if (rayProj < -1e-6) {
        float rpDist = dot(normal, v0 - ray.origin);
        t = rpDist / rayProj;
        p = ray.origin + ray.getDirection() * t;
        return true;
    }
    return false;
}

Vec3 Triangle::hitNormal(const Scene& scene, const TraceHit& hit) const {
    Vec3 result{};
    if (scene.materials[materialIndex].smoothShading) {
        const Vec3& v0N = scene.cacheVertexNormals[v[0]];
        const Vec3& v1N = scene.cacheVertexNormals[v[1]];
        const Vec3& v2N = scene.cacheVertexNormals[v[2]];
        return v0N * (1 - hit.baryU - hit.baryV) + v1N * hit.baryU + v2N * hit.baryV;
    }
    else {
        return normal;
    }
}

void Triangle::translate(const Vec3& translation, std::vector<Vec3>& vertices) const
{
    for (size_t i = 0; i < 3; ++i) {
        vertices[v[i]] += translation;
    }
}

bool Triangle::signOfVolume(const Vec3& a, const Vec3& b, const Vec3& c, const Vec3& d) const {
    return dot(cross(b - a, c - a), d - a) > 0.f;
}

void Triangle::buildAABB(const std::vector<Vec3>& vertices, Vec3* bounds) const
{
    const Vec3& v0 = vertices[v[0]];
    const Vec3& v1 = vertices[v[1]];
    const Vec3& v2 = vertices[v[2]];

    Vec3& min = bounds[0];
    Vec3& max = bounds[1];

    min.x = std::min({ v0.x, v1.x, v2.x });
    min.y = std::min({ v0.y, v1.y, v2.y });
    min.z = std::min({ v0.z, v1.z, v2.z });

    max.x = std::max({ v0.x, v1.x, v2.x });
    max.y = std::max({ v0.y, v1.y, v2.y });
    max.z = std::max({ v0.z, v1.z, v2.z });
}

void to_json(nlohmann::json& j, const Triangle& triangle)
{
    j = nlohmann::json{
        { "v", triangle.v },
        { "materialIndex", triangle.materialIndex } };
}

void from_json(const nlohmann::json& j, Triangle& triangle)
{
    j.at("v").get_to(triangle.v);
    j.at("materialIndex").get_to(triangle.materialIndex);
}
