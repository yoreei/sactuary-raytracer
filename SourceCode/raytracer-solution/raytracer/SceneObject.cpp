#include "include/SceneObject.h"
#include "json.hpp"

nlohmann::ordered_json SceneObject::toJson() const
{
    nlohmann::ordered_json j;
    j["pos"] = pos.toJson();
    j["mat"] = mat.toJson();
    return j;
}

void to_json(nlohmann::json& j, const SceneObject& sceneObj)
{
    j = nlohmann::json{ { "pos", sceneObj.pos },{ "mat", sceneObj.mat } };
}

void from_json(const nlohmann::json& j, SceneObject& sceneObj)
{
    j.at("pos").get_to(sceneObj.pos);
	j.at("mat").get_to(sceneObj.mat);
}
