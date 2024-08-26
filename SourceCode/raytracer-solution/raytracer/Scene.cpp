#pragma warning( disable : 4365 )
#include<limits>

#include "include/Scene.h"
#include "include/AnimationComponent.h"
#include "include/TraceHit.h"
#include "include/Triangle.h"
#include "include/Material.h"
#include "include/CRTTypes.h"
#include "include/CRTSceneIO.h"
#include "include/Light.h"
#include "include/MeshObject.h"
#include "include/Utils.h"
#include "include/AABB.h"
#include "include/Index.h"

bool Scene::isOccluded(const Vec3& start, const Vec3& end) const {
	for (const Triangle& tri : triangles) {
		TraceHit hit{};
		auto& material = materials[tri.materialIndex];
		if (!material.occludes) {
			continue;
		}

		if (tri.fastIntersect(*this, start, end)) {
			return true;
		}
		else {
			continue;
		}
	}
	return false;
}

void Scene::intersect(const Ray& ray, TraceHit& out) const {
	accelStruct.traverse(*this, ray, out);
}

MeshObject& Scene::addObject(
	std::vector<Vec3>& objVertices,
	std::vector<Triangle>& objTriangles,
	std::vector<Vec3>& objUvs)
{
	assert(objVertices.size() > 0);
	assert(objTriangles.size() > 0);
	assert(objUvs.size() > 0);
	size_t vertexIdxPadding = vertices.size();
	Utils::move_back(vertices, objVertices);

	// Pad triangles:
	for (auto& tri : objTriangles) {
		tri.v[0] += vertexIdxPadding;
		tri.v[1] += vertexIdxPadding;
		tri.v[2] += vertexIdxPadding;
	}

	size_t trianglesIdxStart = triangles.size() == 0 ? 0 : triangles.size() - 1;
	Utils::move_back(triangles, objTriangles);
	size_t trianglesIdxEnd = triangles.size() - 1;
	Utils::move_back(uvs, objUvs);

	MeshObject& ref = meshObjects.emplace_back(trianglesIdxStart, trianglesIdxEnd);
	assert(vertices.size() == uvs.size());
	assert(triangles.size() > 0);
	assert(vertices.size() > 0);

	isDirty = true;
	return ref;
}

void Scene::buildVertexNormals() {
	// Vec3{0.f, 0.f, 0.f} is important for summation
	cacheVertexNormals.resize(vertices.size(), Vec3{ 0.f, 0.f, 0.f });
	for (size_t vertIdx = 0; vertIdx < vertices.size(); ++vertIdx) {
		std::vector<size_t> attachedTris = genAttachedTriangles(vertIdx);
		for (size_t triIdx : attachedTris) {
			cacheVertexNormals[vertIdx] += triangles[triIdx].getNormal();
		}
	}

	for (size_t i = 0; i < cacheVertexNormals.size(); ++i) {
		auto& normal = cacheVertexNormals[i];
		normal.normalize();
	}
}

void Scene::buildTriangleNormals()
{
	for (auto& tri : triangles) {
		tri.buildNormal(vertices);
	}
}

std::vector<size_t> Scene::genAttachedTriangles(const size_t vertexIndex) const
{
	std::vector<size_t> triRefs{};
	for (size_t i = 0; i < triangles.size(); ++i) {
		const Triangle& tri = triangles[i];
		if (tri.hasVertex(vertexIndex)) {
			triRefs.push_back(i);
		}
	}
	return triRefs;
}

void Scene::build()
{
	GSceneMetrics.startTimer(Timers::buildScene);
	buildTriangleNormals();
	buildVertexNormals();

	cacheTriangleAABBs.clear();
	cacheTriangleAABBs.resize(triangles.size());

	for (size_t i = 0; i < triangles.size(); ++i) {
		const Triangle& tri = triangles[i];
		tri.buildAABB(vertices, cacheTriangleAABBs[i].bounds);
	}

	std::vector<size_t> triangleRefs = std::vector<size_t>(triangles.size());
	for (size_t i = 0; i < triangles.size(); ++i) {
		triangleRefs[i] = i;
	}

	accelStruct = KDTreeNode(AABB::MakeEnclosingAABB(cacheTriangleAABBs), 0);
	accelStruct.build(std::move(triangleRefs), cacheTriangleAABBs, settings->maxTrianglesPerLeaf, settings->accelTreeMaxDepth);

	std::cout << accelStruct.toString();

	triangleAABBsDirty = false;
	isDirty = false;

	GSceneMetrics.stopTimer(Timers::buildScene);
}

void Scene::updateAnimations() {
	for (auto& [index, animComponent] : lightAnimations) {
		Light& light = lights[index];

		animComponent.intensity.evaluateLerp(light.intensity);
		animComponent.pos.evaluateLerp(light.pos);
		Vec3 newDir;
		animComponent.dir.evaluateSlerp(newDir);
		camera.setDir(newDir);
	}

	for (auto& [index, animComponent] : cameraAnimations) {
		animComponent.pos.evaluateLerp(camera.pos);
		Vec3 newDir;
		animComponent.dir.evaluateSlerp(newDir);
		camera.setDir(newDir);
	}

	for (auto& [index, animComponent] : meshAnimations) {
		MeshObject& meshObject = meshObjects[index];
		Vec3 pos = meshObject.pos;
		animComponent.pos.evaluateLerp(pos);
		meshObject.translateTo(pos);

		std::cerr << "updateAnimations not fully implemented\n";
	}
}

Scene Scene::cut(const std::vector<size_t> trianglesToCut) const
{
	Scene newScene{ sceneName, settings };
	newScene.ambientLightColor = ambientLightColor;
	newScene.bgColor = bgColor;
	newScene.bucketSize = bucketSize;
	newScene.camera = camera;
	newScene.cameraAnimations = cameraAnimations;
	newScene.lightAnimations = lightAnimations;
	newScene.lights = lights;
	newScene.materials = materials;
	// meshObjects will be generated anew
	// newScene.skybox = skybox; todo
	// newScene.textures = textures; todo

	IndexCounter vertexCounter{ vertices.size() };
	std::vector<Vec3> objVertices{};
	std::vector<Vec3> objUvs{};
	std::vector<Triangle> objTriangles{};
	for (const auto& triRef : trianglesToCut) {
		const Triangle& tri = triangles[triRef];

		std::vector<size_t> newVertexRefs{};
		for (const auto& vertexRef : tri.v) {
			int idx = vertexCounter.add(vertexRef);
			if (idx == -1) {
				objVertices.push_back(vertices[vertexRef]);
				objUvs.push_back(uvs[vertexRef]);
				newVertexRefs.push_back(objVertices.size() - 1);
			}
			else {
				newVertexRefs.push_back(idx);
			}
		}

		Triangle newTri{newVertexRefs[0], newVertexRefs[1], newVertexRefs[2], tri.materialIndex };
		objTriangles.push_back(newTri);
	}
	newScene.addObject(objVertices, objTriangles, objUvs);
	newScene.build();
	return newScene;
}
