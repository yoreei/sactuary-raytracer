#include "json.hpp"

#include "include/MeshObject.h"
#include "include/Triangle.h"
#include "include/Scene.h"

MeshObject::MeshObject(size_t l, size_t u) : SceneObject()
{
	triangleIndexes.reserve(u - l);
	for (size_t i = l; i <= u; i++)
	{
		triangleIndexes.push_back(i);
	}
}

MeshObject MeshObject::fromTriangles(const std::vector<size_t>& triangleIndexes)
{
	MeshObject meshObject = MeshObject(0, triangleIndexes.size() - 1);
	meshObject.triangleIndexes = triangleIndexes;
	return meshObject;
}

void MeshObject::translate(const std::vector<Triangle>& triangles, const Vec3& translation, std::vector<Vec3>& vertices)
{
	for (size_t i : triangleIndexes)
	{
		triangles[i].translate(translation, vertices);
	}
}

Material& MeshObject::getMaterial(Scene& scene) const
{
	Triangle& triangle = scene.triangles[triangleIndexes[0]];
	Material& material = scene.materials[triangle.materialIndex];
	return material;
}

void MeshObject::setMaterialIdx(Scene& scene, size_t materialIdx)
{
	for (auto& tri : scene.triangles) {
		tri.materialIndex = materialIdx;
	}
}

void to_json(nlohmann::json& j, const MeshObject& obj)
{
	j = nlohmann::json{ 
		{"triangleIndexes", obj.triangleIndexes},
		{"SceneObject", static_cast<SceneObject>(obj)} };
}

void from_json(const nlohmann::json& j, MeshObject& obj)
{
    j.at("SceneObject").get_to(static_cast<SceneObject&>(obj));
    j.at("triangleIndexes").get_to(obj.triangleIndexes);
}
