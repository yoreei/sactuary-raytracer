#pragma once

#include<memory>

#include "stb_image_write.h"

#include "include/CRTTypes.h"

class Image {
public:
    Image() : Image(0, 0) {}

    Image(size_t _width, size_t _height) :
        width(_width),
        height(_height),
        aspectRatio(static_cast<float>(width) / static_cast<float>(height)),
        data(_width* _height),
        endPixelY(_height),
        endPixelX(_width)
    {}
    std::vector<Color> data;
    // Number of pixels each thread will lock at once.
    size_t bucketSize = 20;

    /* startPixelY, startPixelX, endPixelY, endPixelX are used in conjunction with Settings::debugPixel
       These allow us to shoot a single pixel in a complex scene in Debug mode for fast debuggin */
    size_t startPixelY = 0;
    size_t startPixelX = 0;
    size_t endPixelY;
    size_t endPixelX;

    auto getWidth() const { return width; }
    auto getHeight() const { return height; }
    auto getAspectRatio() const { return aspectRatio; }

    static Image FromBitmap(std::string filePath);

    Color& operator()(size_t x, size_t y)
    {
        if (x >= width || y >= height) throw std::out_of_range("Image::operator(): Index out of range");
        return data[y * width + x];
    }

    const Color& operator()(size_t x, size_t y) const
    {
        if (x >= width || y >= height) throw std::out_of_range("Image::operator(): Index out of range");
        else return data[y * width + x];
    }

    std::vector<Vec2<size_t>> diff(const Image& other) const;

    Image(Image&&) = default;
    Image& operator=(Image&&) = default;

    // Disable copy to prevent accidental expensive copies
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    void writeToPpm(const std::string& filename);

    std::string toPpmString();

    /* @brief Write image to filesystem */
    void writeImage(std::string filename, bool writePng, bool writeBmp) const;

private:
    size_t width;
    size_t height;
    float aspectRatio;

    Vec2<size_t> linearToXY(size_t linearIdx) const;
};
