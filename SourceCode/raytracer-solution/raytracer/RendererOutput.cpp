#include "include/RendererOutput.h"

#include <algorithm>
#include <cassert>

#include "include/Image.h"
#include "include/CRTTypes.h"
#include "include/Settings.h"

RendererOutput::RendererOutput(size_t width, size_t height, const Settings& settings) : width(width), height(height), settings(settings)
{}

void RendererOutput::init()
{
    pixels = std::vector<Shades>(width * height);
    
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

Vec3 flattenShades(const Shades& shades)
{
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
    return result;
}

Image RendererOutput::getFlatImage() const
{
    Image image { width, height };

    if (settings.debugPixel) {

    }
    for (size_t y = startY; y < endY; ++y) {
        for (size_t x = startX; x < endX; ++x) {
            const Shades& shades = pixels[y * width + x];
            if (shades.size() == 0) {
                std::string pixelStr = std::to_string(y) + ", " + std::to_string(x);
                throw std::runtime_error("pixel " + pixelStr + " has no data");
            }

            Vec3 result = flattenShades(shades);
            image(x, y) = Color::fromUnit(result);
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

size_t RendererOutput::pixelsMaxDepth() const
{
    size_t maxDepth = 0;
    for (const auto& pixel : pixels) {
        maxDepth = std::max(pixel.size(), maxDepth);
    }
    return maxDepth;
}
