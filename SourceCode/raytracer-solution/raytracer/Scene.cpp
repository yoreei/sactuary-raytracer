#pragma warning( disable : 4365 )
#include<limits>

#include "include/Scene.h"
#include "include/AnimationComponent.h"
#include "include/TraceHit.h"
#include "include/Triangle.h"
#include "include/Material.h"
#include "include/CRTTypes.h"
#include "include/CRTSceneIO.h"
#include "include/Light.h"
#include "include/MeshObject.h"
#include "include/Utils.h"
#include "include/AABB.h"

bool Scene::isOccluded(const Vec3& start, const Vec3& end) const {
    for (const Triangle& tri : triangles) {
        TraceHit hit{};
        auto& material = materials[tri.materialIndex];
        if (!material.occludes) {
            continue;
        }

        if (tri.fastIntersect(*this, start, end)) {
            return true;
        }
        else {
            continue;
        }
    }
    return false;
}

void Scene::intersect(const Ray& ray, TraceHit& out) const {
    accelStruct.traverse(*this, ray, out);
}

MeshObject& Scene::addObject(
    std::vector<Vec3>& objVertices,
    std::vector<Triangle>& objTriangles,
    std::vector<Vec3>& objVertexNormals,
    std::vector<Vec3>& objUvs)
{
    size_t vertexIdxStart = vertices.size();
    Utils::move_back(vertices, objVertices);
    size_t vertexIdxEnd = vertices.size();

    // Pad triangles:
    for (auto& tri : objTriangles) {
        tri.v[0] += vertexIdxStart;
        tri.v[1] += vertexIdxStart;
        tri.v[2] += vertexIdxStart;
    }

    Utils::move_back(triangles, objTriangles);
    Utils::move_back(vertexNormals, objVertexNormals);
    Utils::move_back(uvs, objUvs);

    MeshObject& ref = meshObjects.emplace_back(vertexIdxStart, vertexIdxEnd);
    assert(vertices.size() == vertexNormals.size());
    assert(vertices.size() == uvs.size());
    assert(triangles.size() > 0);
    assert(vertices.size() > 0);

    isDirty = true;
    return ref;
}

void Scene::build()
{
    GSceneMetrics.startTimer(Timers::buildScene);

    triangleAABBs.clear();
    triangleAABBs.resize(triangles.size());

    for (size_t i = 0; i < triangles.size(); ++i) {
        const Triangle& tri = triangles[i];
        tri.buildAABB(vertices, triangleAABBs[i].bounds);
    }

    std::vector<size_t> triangleRefs = std::vector<size_t>(triangles.size());
    for (size_t i = 0; i < triangles.size(); ++i) {
        triangleRefs[i] = i;
    }

    accelStruct = KDTreeNode(AABB::MakeEnclosingAABB(triangleAABBs), 0);
    accelStruct.build(std::move(triangleRefs), triangleAABBs, settings->maxTrianglesPerLeaf, settings->accelTreeMaxDepth);

    if (settings->forceNoAccelStructure) {
        accelStruct.aabb.expand(Vec3::MakeLowest());
        accelStruct.aabb.expand(Vec3::MakeMax());
    }

    triangleAABBsDirty = false;
    isDirty = false;

    GSceneMetrics.stopTimer(Timers::buildScene);
}

void Scene::updateAnimations() {
    for (auto& [index, animComponent] : lightAnimations) {
        Light& light = lights[index];

        animComponent.intensity.evaluateLerp(light.intensity);
        animComponent.pos.evaluateLerp(light.pos);
        Vec3 newDir;
        animComponent.dir.evaluateSlerp(newDir);
        camera.setDir(newDir);
    }

    for (auto& [index, animComponent] : cameraAnimations) {
        animComponent.pos.evaluateLerp(camera.pos);
        Vec3 newDir;
        animComponent.dir.evaluateSlerp(newDir);
        camera.setDir(newDir);
    }

    for (auto& [index, animComponent] : meshAnimations) {
        MeshObject& meshObject = meshObjects[index];
        Vec3 pos = meshObject.pos;
        animComponent.pos.evaluateLerp(pos);
        meshObject.translateTo(pos);

        std::cerr << "updateAnimations not fully implemented\n";
    }
}
