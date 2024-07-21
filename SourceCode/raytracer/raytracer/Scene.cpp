#pragma warning( disable : 4365 )

#include "Scene.h"
#include "TraceHit.h"
#include "Triangle.h"
#include "Material.h"
#include "CRTTypes.h"
#include "CRTSceneLoader.h"
#include "Light.h"
#include "MeshObject.h"
#include "Utils.h"

/**
* @param start: location vector
* @param end: location vector
* Determine best intersection of ray with scene.
*/
bool Scene::isOccluded(const Vec3& start, const Vec3& end) const {
    // todo remove Vec3 occlusionLine = end - start;
    // todo remove float maxDistanceSq = occlusionLine.lengthSquared();
    // todo remove Ray ray = { start, occlusionLine.getUnit() };
    for (const Triangle& tri : triangles) {
        TraceHit hit{}; // todo move up
        auto& material = materials[tri.materialIndex];
        if (!material.occludes) {
            continue;
        }

        if (tri.boolIntersect(*this, start, end)) {
           return true;
        }
        else {
            continue;
        }
    }
    return false;
}

void Scene::intersect(const Ray& ray, TraceHit& out) const {
    out.t = std::numeric_limits<float>::max();
    TraceHit hit {};
    for (const Triangle& tri : triangles) {
        // TODO: Separate plane intersection & triangle uv intersection tests?
        tri.intersect(*this, ray, hit);
        if (hit.successful() && hit.t < out.t) {
            out = hit;
        }

        metrics.record(toString(hit.type));
    }
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

    return ref;
}

void Scene::merge(const Scene& other)
{
    // not implemented
    other;
    throw std::runtime_error("Not implemented");

    // Will look something like:
    //size_t lightsPadding = moveElement<Light>(lights, scene.lights);
    //size_t meshObjectsPadding = moveElement<MeshObject>(meshObjects, scene.meshObjects);
    //size_t materialsPadding = moveElement<Material>(materials, scene.materials);
    //size_t texturesPadding = moveElement<Texture>(textures, scene.textures);
    //size_t trianglesPadding = moveElement<Triangle>(triangles, scene.triangles);
    //size_t uvsPadding = moveElement<Vec3>(uvs, scene.uvs);
    //size_t vertexNormalsPadding = moveElement<Vec3>(vertexNormals, scene.vertexNormals);
    //size_t verticesPadding = moveElement<Vec3>(vertices, scene.vertices);
}

void Scene::showLightDebug() {
    // TODO: Implement
    throw std::runtime_error("Not implemented");
    //std::vector<Scene> lightObjects;
    //Image _fakeImage {0, 0}; // throwaway image
    //for (const auto& light : lights) {
    //    Scene lightBallScene {"LightBall"};
    //    CRTSceneLoader::loadCrtscene("scenes/props/lightBall.crtscene", lightBallScene, _fakeImage);
    //    assert(lightBallScene.meshObjects.size() == 1);
    //    const Vec3& translation = light.pos;
    //    MeshObject& meshObject = lightBallScene.meshObjects[0];
    //    meshObject.translate(lightBallScene.triangles, translation, lightBallScene.vertices);
    //    lightObjects.push_back(std::move(lightBallScene));
    //}
    //addObjects(lightObjects);
}
