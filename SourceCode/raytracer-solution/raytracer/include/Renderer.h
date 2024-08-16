#pragma once
#include <algorithm>
#include <tuple>
#include <queue>
#include <cmath>
#include <thread>
#include <functional>

#include "perlin-noise.h"

#include "include/Camera.h"
#include "include/CRTTypes.h"
#include "include/Image.h"
#include "include/Scene.h"
#include "include/Triangle.h"
#include "include/RendererOutput.h"


class TraceHit;
class Renderer {
    struct Timers {
        static constexpr const char* all = "RENDER_ALL";
        static constexpr const char* generateQueue = "RENDER_GENERATE_QUEUE";
        static constexpr const char* processQueue = "RENDER_PROCESS_QUEUE";
        static constexpr const char* flatten = "RENDER_FLATTEN";
    };

public:
private:
    // TODO perf: reserve space for the queue
    using TraceQueue = std::queue<TraceTask>;
    std::vector<TraceQueue> queueBuckets {};
    
    // Renderer Output:
    RendererOutput& rendererOutput;

    // Dependencies
    const Scene* scene;
    const Settings* settings;
public:
    Renderer(const Settings* settings, const Scene* scene, RendererOutput& rendererOutput) :
        settings(settings),
        scene(scene),
        rendererOutput(rendererOutput)
        { }

    /*
    * @parameter imageComponents: depth slices of the image, useful for debugging
    */
    void render();
private:

    /* starts multithreading */
    void launchBuckets();

    void workerThread(size_t threadIdx, std::atomic<size_t>& nextQueueIndex);

    void processTraceQueue(TraceQueue& traceQueue);

    void processXData(TraceTask& task, TraceHit& hit, TraceQueue& traceQueue);

    void shadeSky(const TraceTask& task, const TraceHit& hit);

    void shadeSkySimple(const TraceTask& task, const TraceHit& hit);

    void shadeConstant(TraceTask& task, const TraceHit& hit);

    void shadeDiffuse(const TraceTask& task, const TraceHit& hit);

    void shadeReflective(TraceTask& task, const TraceHit& hit, TraceQueue& traceQueue);

    void shadeRefractive(TraceTask& task, TraceHit& hit, TraceQueue& traceQueue);

    /* return: non-clamped hit light color, summed from all lights in the scene */
    Vec3 hitLight(const Vec3& p, const Vec3& n) const;

    /* @param overexposure: x,y,z > 1.f
    *  @return: x,y,z clamped to 1.f */
    Vec3 clampOverexposure(const Vec3& overexposed) const;

    /* @return: choose schlick implementation */
    float schlickApprox(const Vec3& i, const Vec3& n, float n1, float n2) const {
        n1; // clear unused param warning
        n2;
        return schlickApproxVladi(i, n);
    }
    /* @return: fresnel factor */
    float schlickApproxWikipedia(const Vec3& i, const Vec3& n, float n1, float n2) const;

    /* @return: fresnel factor */
    float schlickApproxVladi(const Vec3& i, const Vec3& n) const;

    Color shadePosition(const TraceHit& hit) const;

    Color shadePerlin(Vec3 p) const;

    void shadeBary(const TraceTask& task, const TraceHit& hit);

    void shadeUv(const TraceTask& task, const TraceHit& hit);

    void shadeNormal(TraceTask& task, const TraceHit& hit);

    /* hw3. For debugging camera Ray generation */
    Color traceImagePlane(const Ray& ray) const;
};
