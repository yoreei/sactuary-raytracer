#pragma once
#include <vector>
#include <queue>

#include "include/CRTTypes.h"
#include "include/TraceHit.h"

enum class BlendType {
    NORMAL, /* Source Over */
    ADDITIVE,
};
/* A single ray's contribution to the final pixel color */
class Shade {
public:
    Vec3 color = { 0.f, 0.f, 0.f };
    float weight = 1.f;
    BlendType blendType = BlendType::NORMAL;
};

class Image;
using Shades = std::vector<Shade>;
using TraceHits = std::vector<TraceHit>;

class Settings;
/* Records each shade's contribution to a pixel. `flatten(..)` to produce the final image. */
class RendererOutput
{
public:
    RendererOutput(const Settings& settings);
    void init();

    Image getFlatImage() const;
    std::vector<Image> getDepthImages() const;
    std::vector<size_t> getVisibleTriangleIds(size_t maxTriangles) const;
    Color flattenPixel(size_t x, size_t y) const;

    void addSample(const TraceTask& task, const Vec3 color, const TraceHit& hit, BlendType blendType = BlendType::NORMAL);

    // Data
    std::vector<Shades> pixels{};
    std::vector<TraceHits> traceHits{};

    // Parameters
    size_t width = 0;
    size_t height = 0;
    size_t endX = 0;
    size_t endY = 0;
    size_t startX = 0;
    size_t startY = 0;
    const Settings& settings;
private:
    /* @brief max depth across all pixels */

    size_t pixelsMaxDepth() const;
    bool recordHits = false;
};

