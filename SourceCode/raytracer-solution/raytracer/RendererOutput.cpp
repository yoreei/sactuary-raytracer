#include "include/RendererOutput.h"

#include <algorithm>
#include <cassert>

#include "include/Image.h"
#include "include/CRTTypes.h"
#include "include/Settings.h"
#include "include/Index.h"

RendererOutput::RendererOutput(const Settings& settings): settings(settings)
{}

void RendererOutput::init()
{
    width = settings.resolutionX;
    height = settings.resolutionY;
    startX = 0;
    startY = 0;
    endX = width;
    endY = height;

    pixels = std::vector<Shades>(width * height);
    
    if (settings.debugPixel) {
        startX = settings.debugPixelX;
        startY = settings.debugPixelY;
        endX = settings.debugPixelX + 1;
        endY = settings.debugPixelY + 1;
    }

    if (settings.pruneInvisible) {
        traceHits = std::vector<TraceHits>(width * height);
        recordHits = true;
    }
}

void RendererOutput::addSample(const TraceTask& task, const Vec3 color, const TraceHit& hit, BlendType blendType)
{
    if (task.pixelX >= endX || task.pixelY >= endY)
    {
        throw std::out_of_range("");
    }
    Shades& shades = pixels[task.pixelY * width + task.pixelX];

    shades.emplace_back(color, task.weight, blendType);

    if (recordHits) {
		TraceHits& pixelTraceHits = traceHits[task.pixelY * width + task.pixelX];
        pixelTraceHits.push_back(hit);
    }
}

Color RendererOutput::flattenPixel(size_t x, size_t y) const
{
	const Shades& shades = pixels[y * width + x];
	if (shades.size() == 0) {
		std::string pixelStr = std::to_string(y) + ", " + std::to_string(x);
		throw std::runtime_error("pixel " + pixelStr + " has no data");
	}

    Vec3 result;
    result = shades[0].color;
    for (size_t sIdx = 1; sIdx < shades.size(); ++sIdx) {
        const Shade& shade = shades[sIdx];

        if (shade.blendType == BlendType::NORMAL) {
            result = lerp(result, shade.color, shade.weight);
        }

        else if (shade.blendType == BlendType::ADDITIVE) {
            result = (result + shade.color);
            result.clamp(0.f, 1.f);
        }
        else {
            throw std::runtime_error("unknown BlendType");
        }
    }
    return Color::fromUnit(result);
}

Image RendererOutput::getFlatImage() const
{
    Image image { width, height };

    for (size_t y = startY; y < endY; ++y) {
        for (size_t x = startX; x < endX; ++x) {
            image(x, y) = flattenPixel(x, y);
        }
    }
    return image;
}

std::vector<Image> RendererOutput::getDepthImages() const
{
    std::vector<Image> images{};

    // Prepare images vector
    size_t maxDepth = pixelsMaxDepth();
    images.reserve(maxDepth);
    for (size_t i = 0; i < maxDepth; ++i) {
        images.emplace_back(width, height);
    }

    // From array of stacks (pixels) To stacks of arrays (images)
    for (size_t pIdx = 0; pIdx < width * height; ++pIdx) {
        const Shades& shades = pixels[pIdx];

        for (size_t sIdx = 0; sIdx < shades.size(); ++sIdx) {
            const Shade& shade = shades[sIdx];
            images[sIdx].data[pIdx] = Color::fromUnit(shade.color);
        }
    }

    return images;
}

std::vector<size_t> RendererOutput::getVisibleTriangleIds(size_t maxTriangles) const
{
    if (!recordHits) {
        throw std::runtime_error("Hits must be recorded to calculate visible triangles!");
    }

    std::vector<size_t> triangleIds{};
    IndexMap map{ maxTriangles };

    for (const auto& pixelTraceHits : traceHits) {
        for (const auto& depthTraceHit : pixelTraceHits) {
            map.add(depthTraceHit.triRef, triangleIds);
        }
    }

    return triangleIds;
}

size_t RendererOutput::pixelsMaxDepth() const
{
    size_t maxDepth = 0;
    for (const auto& pixel : pixels) {
        maxDepth = std::max(pixel.size(), maxDepth);
    }
    return maxDepth;
}
