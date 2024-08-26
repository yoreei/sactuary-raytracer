#pragma once
#include "include/CRTTypes.h"

#include "json_fwd.h"

class TraceHit;
class Scene;
class Material
{
public:
    enum Type
    {
        VOID, // No material
        DIFFUSE,
        CONSTANT,
        REFLECTIVE,
        REFRACTIVE,
        DEBUG,
        DEBUG_NORMAL,
        DEBUG_UV,
        DEBUG_BARY
    };

    Material() = default;
    Material(const Vec3& albedo, bool smoothShading, Type type) : albedo(albedo), smoothShading(smoothShading), type(type) {}

    Type type = Type::DIFFUSE;

    // reflectivity + transparency + diffuseness = 1.f
    Vec3 albedo{ 0.f, 1.f, 0.f };
    float reflectivity = 0.f;
    float transparency = 0.f;
    float diffuseness = 0.f;

    float ior = 0.f;
    size_t textureIdx = 0;
    // occludes: Does it cast shadows?
    bool occludes = true;
    bool hasTexture = false;
    bool smoothShading = false;

    static Type TypeFromString(const std::string& type);

    static std::string StringFromType(const Material::Type& type);

    std::string toString() const;

    Vec3 getAlbedo(const Scene& scene, const TraceHit& hit) const;

    void setAlbedo(const Vec3& _albedo) { this->albedo = _albedo; }
private:
};

inline std::ostream& operator<<(std::ostream& os, const Material& material) {
    os << material.toString();
    return os;
}

void to_json(nlohmann::json& j, const Material& material);
void from_json(const nlohmann::json& j, Material& material);

