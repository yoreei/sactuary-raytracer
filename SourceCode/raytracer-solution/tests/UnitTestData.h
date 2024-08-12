#pragma once

#include<string>

#include "include/Camera.h"
#include "include/Scene.h"
#include "include/TestUtils.h"
#include "include/Material.h"

namespace UnitTestData {

    void loadCamera1(Camera& camera)
    {
        Vec3 camPos = { 0.f,0.f,0.f };
        Vec3 dir = { 0.f, 0.f, -1.f };
        dir.normalize();

        camera = Camera{};
        camera.setDir(dir);
        camera.fov = 90.f;
    }

    void loadScene1(Scene& scene)
    {
        scene.sceneName = "scene1";
        scene.vertices = {
        {-1.75f, -1.75f, -3.f},
        {1.75f, -1.75f, -3.f},
        {0.f, 1.75f, -3.f},
        };
        size_t materialIdx = 0;

        scene.triangles = {
            {scene.vertices, 0, 1, 2, materialIdx}
        };

        Camera camera{};
        loadCamera1(camera);
        scene.camera = camera;
    }

} // namespace UnitTestData
