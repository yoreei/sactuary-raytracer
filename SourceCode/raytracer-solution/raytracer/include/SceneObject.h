#pragma once
#include <memory>

#include "json_fwd.h"

#include "include/CRTTypes.h"

class SceneObject {
public:
    SceneObject() = default;
    SceneObject(const Vec3& pos, const Matrix3x3& mat) : pos(pos), mat(mat) {
    }
    Vec3 pos{ 0.f, 0.f, 0.f };
    Matrix3x3 mat = SceneObject::DefaultMatrix;

    void rotate(Vec3 rotVec)
    {
        rotVec;
        throw std::runtime_error("SceneObject::rotate not implemented");
    }

    void translateTo(Vec3 absPos)
    {
        absPos;
        throw std::runtime_error("SceneObject::translateTo not implemented");
    }

    // TODO reomove toJson and leave to_json
    nlohmann::ordered_json toJson() const;
    inline const static Matrix3x3 DefaultMatrix = { {1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 0.f, 0.f, -1.f } };
};

void to_json(nlohmann::json& j, const SceneObject& sceneObj);
void from_json(const nlohmann::json& j, SceneObject& sceneObj);
