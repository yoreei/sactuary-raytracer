#pragma once

#include <fstream>
#include <unordered_map>

#include "json.hpp"

#include "include/KDTreeNode.h"
#include "include/AnimationComponent.h"
#include "include/CRTTypes.h"
#include "include/Camera.h"
#include "include/Cubemap.h"
#include "include/Image.h"
#include "include/Light.h"
#include "include/MeshObject.h"
#include "include/Material.h"
#include "include/Metrics.h"
#include "include/Settings.h"
#include "include/Triangle.h"
#include "include/Texture.h"
#include "include/Globals.h"

using json = nlohmann::json;

class TraceHit;
class Scene
{
public:
    Scene(const std::string& name, const Settings* settings) : sceneName(name), settings(settings) {}

    Scene(Scene&&) noexcept = default;
    Scene& operator=(Scene&&) noexcept = default;

    Scene(const Scene&) = delete;
    Scene& operator=(const Scene&) = delete;

    // Properties
    size_t bucketSize = 20;
    std::string sceneName = "";
    Camera camera{};
    const Settings* settings;
    Cubemap skybox{};
    Vec3 bgColor = { 0.f, 0.f, 0.f };
    bool useSkybox = false;
    Vec3 ambientLightColor = { 0.f, 0.f, 0.f };

    // Entities
    std::vector<Light> lights {};
    std::vector<MeshObject> meshObjects {}; /* meshObjects reference trinagles. Triangles reference vertices */
    std::vector<Material> materials {};
    std::vector<Texture> textures {};
    std::vector<Triangle> triangles {};
    std::vector<Vec3> uvs {};
    std::vector<Vec3> vertices {};

    std::vector<AABB> cacheTriangleAABBs {};
    std::vector<Vec3> cacheVertexNormals {};

    // Components
    std::unordered_map<int, AnimationComponent> lightAnimations {}; // Key: index in lights vector
    std::unordered_map<int, AnimationComponent> meshAnimations {};  // Key: index in meshObjects vector
    std::unordered_map<int, AnimationComponent> cameraAnimations {}; // size <= 1
    // IMPORTANT: update `Scene::addObjects()` if adding new members
    // IMPORTANT: keep alphabetical order

    bool isOccluded(const Vec3& start, const Vec3& end) const;
    void intersect(const Ray& ray, TraceHit& out) const;

    /* @brief: Marks scene dirty. Do not forget to `build` the scene after addObject! */
    MeshObject& addObject(
        std::vector<Vec3>& objVertices,
        std::vector<Triangle>& objTriangles,
        std::vector<Vec3>& objUvs);

	void buildVertexNormals();

    void buildTriangleNormals();

    std::vector<size_t> genAttachedTriangles(const size_t vertexIndex) const;

    /* @brief: build acceleration structures. Marks scene clean. See `isDirty` */
    void build();

    bool getIsDirty() const { return isDirty; }

    void updateAnimations();

    Scene cut(const std::vector<size_t> trianglesToCut) const;

private:
    KDTreeNode accelStruct{};
    bool isDirty = true; /* Scene is dirty if objects are added or removed */
    bool triangleAABBsDirty = true;

    /* Metrics Timers for [start/stop]Timer*/
    struct Timers {
        static constexpr const char* buildScene = "buildScene";
    };
};
