#include "include/Scripts.h"
#include "include/Scene.h"
#include "include/CRTTypes.h"
#include "include/Globals.h"

static float fps;
static float duration;
static float endFrame;
void Scripts::onSceneLoaded(Scene& scene) {
    if (scene.sceneName == "day" ||
        scene.sceneName == "day2" ||
        scene.sceneName == "night" ||
        scene.sceneName == "night2")
    {
        // set start frame
        GFrameNumber = 0;

        fps = 60; // Frames per second
        duration = 7.0f; // Duration of the animation in seconds
        endFrame = fps * duration;
        GEndFrame = uint64_t(endFrame);
    }
}

void Scripts::onTick(Scene& scene) {
    if (scene.sceneName == "day") {

        // Parameters for the circular path
        float startAngle = PI / 4.f; // (radians)
        float endAngle = 5 * PI / 4.f; // (radians)
        float startRadius = 16.0f; // Radius of the circle
        float endRadius = 7.f;
        float startCamHeight = 5.f;
        float endCamHeight = -4.f;

        float t = GFrameNumber / endFrame; // Normalized time [0, 1]

        float angle = startAngle * (1.f - t) + endAngle * t;
        float radius = startRadius * (1.f - t) + endRadius * t;
        float camHeight = startCamHeight * (1.f - t) + endCamHeight * t;

        Vec3 position = { radius * std::cos(angle), camHeight, radius * std::sin(angle) };

        scene.camera.pos = position;
        scene.camera.lookAt({ 0.f, 0.f, 0.f });
    }
    if (scene.sceneName == "day2") {

        // Parameters for the circular path
        float startAngle = PI; // (radians)
        float endAngle = PI / 4.f; // (radians)
        float startRadius = 14.0f; // Radius of the circle
        float endRadius = 8.f;
        float startCamHeight = 2.f;
        float endCamHeight = -4.f;

        float t = GFrameNumber / endFrame; // Normalized time [0, 1]

        float angle = startAngle * (1.f - t) + endAngle * t;
        float radius = startRadius * (1.f - t) + endRadius * t;
        float camHeight = startCamHeight * (1.f - t) + endCamHeight * t;

        Vec3 position = { radius * std::cos(angle), camHeight, radius * std::sin(angle) };

        scene.camera.pos = position;
        scene.camera.lookAt({ 0.f, 0.f, 0.f });
    }


    if (scene.sceneName == "night") {
        fps = 1; // Frames per second
        duration = 5.0f; // Duration of the animation in seconds
        endFrame = fps * duration;
        GEndFrame = uint64_t(endFrame);

        // Parameters for the circular path
        float startFov = 30.f; // (degrees)
        float endFov = 90.f; // (degrees)

        float startAngle = -PI / 4.f; // (radians)
        float endAngle = -PI / 2.f; // (radians)
        float startRadius = 13.0f; // Radius of the circle
        float endRadius = 13.f;
        float startCamHeight = 8.f;
        float endCamHeight = 8.f;

        float t = GFrameNumber / endFrame; // Normalized time [0, 1]

        float angle = startAngle * (1.f - t) + endAngle * t;
        float radius = startRadius * (1.f - t) + endRadius * t;
        float camHeight = startCamHeight * (1.f - t) + endCamHeight * t;
        float fov = startFov * (1.f - t) + endFov * t;

        Vec3 position = { radius * std::cos(angle), camHeight, radius * std::sin(angle) };

        //fov;
        scene.camera.setFov(fov);
        scene.camera.pos = position;
        scene.camera.lookAt({ 0.f, 0.f, 0.f });
    }

    if (scene.sceneName == "night2") {
        fps = 1; // Frames per second
        duration = 8.0f; // Duration of the animation in seconds
        endFrame = fps * duration;
        GEndFrame = uint64_t(endFrame);

        // Parameters for the circular path
        float startFov = 30.f; // (degrees)
        float endFov = 90.f; // (degrees)

        Vec3 startLightPos{ -12.f, 5.f, 4.f };
        Vec3 endLightPos{ -12.f, 5.f, -4.f };
        float lightEndFrame = 2.f * fps;
        Vec3 startLookAt{ 0.f, 3.f, 0.f };
        Vec3 endLookAt{ 0.f, 12.f, 0.f };
        float startAngle = PI; // (radians)
        float endAngle = PI; // (radians)
        float startRadius = 13.0f; // Radius of the circle
        float endRadius = 9.f;
        float startCamHeight = 10.f;
        float endCamHeight = 4.f;

        float t = GFrameNumber / endFrame; // Normalized time [0, 1]
        float lightT = GFrameNumber / lightEndFrame;
        lightT = std::clamp(lightT, 0.f, 1.f);

        float angle = startAngle * (1.f - t) + endAngle * t;
        float radius = startRadius * (1.f - t) + endRadius * t;
        float camHeight = startCamHeight * (1.f - t) + endCamHeight * t;
        float fov = startFov * (1.f - t) + endFov * t;

        Vec3 lightPos = lerp(startLightPos, endLightPos, lightT);
        Vec3 lookAt = lerp(startLookAt, endLookAt, t);
        Vec3 position = { radius * std::cos(angle), camHeight, radius * std::sin(angle) };

        scene.lights[4].pos = lightPos;
        scene.camera.setFov(fov);
        scene.camera.pos = position;
        scene.camera.lookAt(lookAt);
    }
}
