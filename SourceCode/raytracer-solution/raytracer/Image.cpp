#include "include/Image.h"

#include "stb_image.h"

#include <fstream>

std::vector<Vec2<size_t>> Image::diff(const Image& other) const
{
    std::vector<Vec2<size_t>> diffVec{};
    if (getWidth() != other.getWidth() || getHeight() != other.getHeight()) { throw std::invalid_argument("image dimensions must match"); }
    for (size_t i = 0; i < getWidth() * getHeight(); ++i) {
        if (data[i] - other.data[i] != Color{ 0, 0, 0 }) {
            diffVec.emplace_back(linearToXY(i));
        }
    }

    return diffVec;
}

void Image::writeToPpm(const std::string& filename)
{
    std::ofstream ppmFileStream(filename, std::ios::out | std::ios::binary);
    if (!ppmFileStream.is_open()) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return;
    }

    ppmFileStream << "P3\n"; ppmFileStream << width << " " << height << "\n";
    ppmFileStream << Color::maxColorComponent << "\n";

    for (size_t rowIdx = 0; rowIdx < height; ++rowIdx) {
        for (int colIdx = 0; colIdx < width; ++colIdx) {
            const Color& color = data[rowIdx * width + colIdx];
            ppmFileStream << (int)color.r << " " << (int)color.g << " " << (int)color.b << "\t";
        }
        ppmFileStream << "\n";
    }

    ppmFileStream.close();
    if (ppmFileStream.fail()) {
        std::cerr << "Failed to write to file: " << filename << std::endl;
    }
}

std::string Image::toPpmString() {
    std::string result;
    result.reserve(20 + (width * height * 12));

    result += "P3\n";
    result += std::to_string(width) + " " + std::to_string(height) + "\n";
    result += std::to_string(Color::maxColorComponent) + "\n";

    for (size_t rowIdx = 0; rowIdx < height; ++rowIdx) {
        for (size_t colIdx = 0; colIdx < width; ++colIdx) {
            const Color& color = data[rowIdx * width + colIdx];
            result += std::to_string(color.r) + " " + std::to_string(color.g) + " " + std::to_string(color.b) + "\t";
        }
        result += "\n";
    }

    return result;
}

void Image::writeImage(std::string filename, bool writePng, bool writeBmp) const {
    std::vector<uint8_t> pngData(width * height * 3);
    for (size_t y = 0; y < height; ++y) {
        for (size_t x = 0; x < width; ++x) {
            const Color& color = data[y * width + x];
            pngData[3 * (y * width + x) + 0] = color.r;
            pngData[3 * (y * width + x) + 1] = color.g;
            pngData[3 * (y * width + x) + 2] = color.b;
        }
    }
    int channels = 3;
    int intWidth = static_cast<int>(width);
    int intHeight = static_cast<int>(height);

    if (writePng) {
        int stride_bytes = static_cast<int>(width) * 3;
        std::string filenameWithExt = filename + ".png";
        int result = stbi_write_png(filenameWithExt.c_str(), intWidth, intHeight, channels, pngData.data(), stride_bytes);
        if (result == 0) {
            throw std::runtime_error("Failed to write PNG file");
        }
    }
    if (writeBmp) {
        std::string filenameWithExt = filename + ".bmp";
        int result = stbi_write_bmp(filenameWithExt.c_str(), intWidth, intHeight, channels, pngData.data());
        if (result == 0) {
            throw std::runtime_error("Failed to write BMP file");
        }
    }

}

Vec2<size_t> Image::linearToXY(size_t linearIdx) const
{
    if (linearIdx >= width * height) {
        throw std::out_of_range("linearIdx is out of bounds");
    }

    return Vec2<size_t>(linearIdx % width, linearIdx / width);
}

Image Image::FromBitmap(std::string filePath)
{
    std::ifstream file(filePath);
    if (!file) {
        throw std::runtime_error("Failed to load image");
    }

    int width, height, channels;
    int ok = stbi_info(filePath.c_str(), &width, &height, &channels);
    if (ok != 1) {
        throw std::runtime_error("Failed to load image: " + std::string(stbi_failure_reason()));
    }
    Image img{ size_t(width), size_t(height) };

    unsigned char* stbi_ptr = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (stbi_ptr == nullptr || channels < 3 || channels > 4) {
        throw std::runtime_error("Failed to load image: " + std::string(stbi_failure_reason()));
    }

    for (size_t i = 0; i < width * height; ++i)
    {
        img.data[i].r = stbi_ptr[i * channels];
        img.data[i].g = stbi_ptr[i * channels + 1];
        img.data[i].b = stbi_ptr[i * channels + 2];
    }

    stbi_image_free(stbi_ptr);

    return img;
}

