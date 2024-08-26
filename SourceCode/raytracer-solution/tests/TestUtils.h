#pragma once
#include "include/CRTTypes.h"
#include "include/Triangle.h"

Triangle& addTriangle(Scene& scene, const Vec3& v0, const Vec3& v1, const Vec3& v2)
{
    auto& vs = scene.vertices;
    auto& tris = scene.triangles;

    size_t startIdx = vs.size();
    vs.push_back(v0);
    vs.push_back(v1);
    vs.push_back(v2);

    size_t materialIdx = 0;
    Triangle t{ vs, startIdx, startIdx + 1, startIdx + 2, materialIdx };
    tris.push_back(t);

    return tris.back();
}

void resetScene(Scene& scene)
{
    scene.vertices.clear();
    scene.triangles.clear();
    scene.materials.clear();
    scene.meshObjects.clear();
    scene.cacheVertexNormals.clear();
    scene.lights.clear();
}
