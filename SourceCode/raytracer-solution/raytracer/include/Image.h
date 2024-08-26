#pragma once

#include<memory>
#include<filesystem>

#include "stb_image_write.h"

#include "include/CRTTypes.h"

class ImageDiff;
class Image {
public:
    Image() : Image(0, 0) {}

    Image(size_t _width, size_t _height) :
        width(_width),
        height(_height),
        data(_width* _height)
    {}
    std::vector<Color> data;

    auto getWidth() const { return width; }
    auto getHeight() const { return height; }
    float getAspectRatio() const { return static_cast<float>(width) / static_cast<float>(height); }

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

    ImageDiff diff(const Image& other) const;

    Image(Image&&) = default;
    Image& operator=(Image&&) = default;

    // Disable copy to prevent accidental expensive copies
    Image(const Image&) = delete;
    Image& operator=(const Image&) = delete;

    void writeToPpm(const std::string& filename);

    std::string toPpmString();

    /* @brief Write image to filesystem */
    void writeToFile(std::filesystem::path filename) const;

private:
    size_t width;
    size_t height;

    Vec2<size_t> linearToXY(size_t linearIdx) const;
};

class ImageDiff {
public:
    ImageDiff(size_t width, size_t height) : width(width), height(height) {}
    size_t width = 0;
    size_t height = 0;
    std::vector<Vec2<size_t>> diffs{};

    void writeToFile(std::filesystem::path filename) const;

    // Disable copy to prevent accidental expensive copies
    ImageDiff(const ImageDiff&) = delete;
    ImageDiff& operator=(const ImageDiff&) = delete;

    ImageDiff(ImageDiff&&) = default;
    ImageDiff& operator=(ImageDiff&&) = default;
};

