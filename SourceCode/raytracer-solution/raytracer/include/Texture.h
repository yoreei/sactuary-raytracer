#pragma once
#include <string>

#include "json_fwd.h"

#include "include/CRTTypes.h"
#include "include/Image.h"

enum class TextureType {
    SOLID_COLOR,
    EDGES,
    CHECKER,
    BITMAP,
};

class TraceHit;
class Scene;
class Texture {
public:
    Texture() = default; // required for nlohmann::json deserialization
    Texture(std::string& name, TextureType type) : name(name), type(type) {}
    TextureType type = TextureType::SOLID_COLOR;
    std::string name = "defaultTexture";
    Vec3 color1{ 0.f, 0.f, 0.f }; // edge_color, color_A, albedo
    Vec3 color2{ 0.f, 0.f, 0.f }; // inner_color, color_B
    float textureSize = 1.f; // edge_width, square_size
    std::string filePath = ""; // only BITMAP
    Image bitmap{};

    static TextureType TypeFromString(const std::string& type);
    static std::string StringFromType(const TextureType& type);
    Vec3 getAlbedo(const TraceHit& hit) const;

    // disable copy. Enable move:
    Texture(Texture&&) = default;
    Texture& operator=(Texture&&) = default;

    Texture(const Texture&) = delete;
    Texture& operator=(const Texture&) = delete;

};

void to_json(nlohmann::json& j, const Texture& tex);
void from_json(const nlohmann::json& j, Texture& tex);

