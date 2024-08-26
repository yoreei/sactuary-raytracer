#pragma once
#include <algorithm>
#include <numbers>

#include "json_fwd.h"

#include "include/CRTTypes.h"
#include "include/SceneObject.h"

enum class LightType {
    SUN,
    POINT,
    INVALID,
};

class Scene;
class Light : public SceneObject {
public:
    Light() = default;
    static Light MakeSun(const Vec3& direction, float intensity, const Vec3& color)
    {
        if (!direction.isUnit()) { throw std::runtime_error("direction not normalized"); }
        Vec3 pos = { 0.f, 0.f, 0.f }; // does not matter for type SUN
        Light light = Light(LightType::SUN, pos, intensity, color);
        light.direction = direction;
        return light;
    }
    static Light MakePoint(const Vec3& position, float intensity, const Vec3& color)
    {
        return Light(LightType::POINT, position, intensity, color);
    }
    static LightType lightTypeFromString(const std::string& str);
    static std::string stringFromLightType(const LightType lightType);
    /* Output:vector representing RGB contribution of the light source */
    Vec3 lightContrib(const Scene& scene, const Vec3& point, const Vec3& normal) const;

    float intensity = 0;
    Vec3 color{ 1.f, 1.f, 1.f };
    LightType type = LightType::INVALID;
    Vec3 direction{ 0.f, -1.f, 0.f };

private:
    Light(LightType type, const Vec3& pos, float intensity, const Vec3& color)
        :type(type), SceneObject(pos, Matrix3x3()), intensity(intensity), color(color) {}
};

void to_json(nlohmann::json& j, const Light& light);
void from_json(const nlohmann::json& j, Light& light);

