#pragma warning( disable : 4365 )

#include <fstream>
#include <limits>
#include <memory>

#include "json.hpp"

#include "include/CRTSceneIO.h"
#include "include/Scene.h"
#include "include/Image.h"
#include "include/Material.h"
#include "include/Texture.h"
#include "include/Settings.h"
#include "include/Utils.h"

using json = nlohmann::json;

void CRTSceneIO::write(const Settings& settings, const Scene& scene)
{
    Path path = settings.getSceneOutput() / scene.sceneName;

    std::string pathStr = path.string();
    std::cout << "Writing Scene: " << pathStr << std::endl;
    std::ofstream file(pathStr);
    if (!file) { throw std::runtime_error("Failed to load CRTScene file: " + pathStr); }

    throw std::runtime_error("not implemented");
}

void CRTSceneIO::load(const Settings& settings, const std::filesystem::path& filePath, Scene& scene)
{
    // Create a clean scene:
    std::string sceneName = filePath.filename().string();
    scene = Scene{sceneName, &settings};

    std::string filePathStr = filePath.string();
    std::ifstream file(filePathStr);
    if (!file) { throw std::runtime_error("Failed to load CRTScene file: " + filePathStr); }

    json j;
    file >> j;

    std::cout << "Loading Scene: " << filePathStr << std::endl;

    parseSettings(j, scene, settings);
    parseImageSettings(j, scene, settings);
    parseCameraSettings(j, scene);

    std::map<std::string, size_t> idxFromTextureName;
    parseTextures(j, scene, settings, idxFromTextureName);
    parseMaterials(j, scene, idxFromTextureName);

    parseObjects(j, scene);
    parseLight(j, scene);

    scene.build();
}

void CRTSceneIO::parseLight(const json& j, Scene& scene)
{
    warnIfMissing(j, "lights");
    if (!j.contains("lights")) { return; }

    for (const auto& jLight : j.at("lights")) {
        float intensity = jLight.at("intensity");
        Vec3 color = Utils::jsonGetDefault<Vec3>(jLight, "color", {1.f, 1.f, 1.f});
        std::string typeStr = Utils::jsonGetDefault<std::string>(jLight, "type", "point");
        LightType type = Light::lightTypeFromString(typeStr);
        Light light;

        if (type == LightType::SUN) {
            const auto& jDir = jLight.at("direction");
            Vec3 dir{ jDir[0], jDir[1], jDir[2] };
            dir.normalize();
            light = Light::MakeSun(dir, intensity, color);
        }
        else if (type == LightType::POINT) {
            const auto& jPos = jLight.at("position");
            Vec3 pos{ jPos[0], jPos[1], jPos[2] };
            light = Light::MakePoint(pos, intensity, color);
        }
        else {
            throw std::runtime_error("unknown light type");
        }

        scene.lights.push_back(light);
    }
}

void CRTSceneIO::parseSettings(const json& j, Scene& scene, const Settings& settings) {
    const auto& jSettings = j.at("settings");
    const auto& jBgColor = jSettings.at("background_color");
    scene.bgColor = Vec3{ jBgColor[0], jBgColor[1], jBgColor[2] };

    if (jSettings.contains("skybox")) {
        std::string skybox = jSettings.at("skybox");
        scene.useSkybox = true;
        scene.skybox.images[0] = Image::FromBitmap(settings.sceneLibraryDir + "/skybox/" + skybox + "/0001.png");
        scene.skybox.images[1] = Image::FromBitmap(settings.sceneLibraryDir + "/skybox/" + skybox + "/0002.png");
        scene.skybox.images[2] = Image::FromBitmap(settings.sceneLibraryDir + "/skybox/" + skybox + "/0003.png");
        scene.skybox.images[3] = Image::FromBitmap(settings.sceneLibraryDir + "/skybox/" + skybox + "/0004.png");
        scene.skybox.images[4] = Image::FromBitmap(settings.sceneLibraryDir + "/skybox/" + skybox + "/0005.png");
        scene.skybox.images[5] = Image::FromBitmap(settings.sceneLibraryDir + "/skybox/" + skybox + "/0006.png");
    }

    if (scene.useSkybox) scene.ambientLightColor = scene.skybox.calculateAmbientColor();
    else { scene.ambientLightColor = scene.bgColor; }
}

void CRTSceneIO::parseImageSettings(const json& j, Scene& scene, const Settings& settings)
{
    const auto& jImgSettings = j.at("settings").at("image_settings");

    if (jImgSettings.contains("bucket_size")) {
        scene.bucketSize = jImgSettings.at("bucket_size").get<size_t>();
    }

    if (settings.forceSingleThreaded || settings.debugPixel) {
        scene.bucketSize = std::numeric_limits<size_t>::max();
    }

}

void CRTSceneIO::parseCameraSettings(const json& j, Scene& scene) {
    const auto& jCam = j.at("camera");
    const auto& jCamPos = jCam.at("position");
    Vec3 camPos{ jCamPos.at(0), jCamPos.at(1), jCamPos.at(2) };

    if (jCam.contains("matrix")) {
        const auto& jRotateMat = jCam.at("matrix");
        Matrix3x3 rotateMat{ jRotateMat.get<std::vector<float>>() };
        // NB: CRTScene format gives us the inverse direction of the camera.
        Matrix3x3 camMat = Camera::DefaultMatrix * rotateMat;
        scene.camera = Camera{ 90.f, camPos, camMat };
    }
    else if (jCam.contains("dir")) {
        Vec3 dir = Vec3FromJson(jCam.at("dir"));

        scene.camera = Camera{ 90.f, camPos, Camera::DefaultMatrix };
        scene.camera.setDir(dir);
    }
    else { throw std::runtime_error("camera requires either matrix or dir"); }
}

void CRTSceneIO::parseTextures(const json& j, Scene& scene, const Settings& settings, std::map<std::string, size_t>& idxFromTextureName)
{
    if (!j.contains("textures")) { return; }

    auto& jTextures = j.at("textures");
    for (size_t i = 0; i < jTextures.size(); ++i) {
        const auto& jTexture = jTextures[i];
        std::string name = jTexture.at("name");
        std::string typeStr = jTexture.at("type");
        TextureType type = Texture::TypeFromString(typeStr);
        Texture tex{ name, type };

        assignIfExists<Vec3>(jTexture, "albedo", tex.color1);
        assignIfExists<Vec3>(jTexture, "edge_color", tex.color1);
        assignIfExists<Vec3>(jTexture, "color_A", tex.color1);
        assignIfExists<Vec3>(jTexture, "inner_color", tex.color2);
        assignIfExists<Vec3>(jTexture, "color_B", tex.color2);
        assignIfExists<float>(jTexture, "edge_width", tex.textureSize);
        assignIfExists<float>(jTexture, "square_size", tex.textureSize);
        assignIfExists<std::string>(jTexture, "file_path", tex.filePath);
        tex.filePath = settings.sceneLibraryDir + "/" + settings.projectDir + "/" + tex.filePath;

        if (type == TextureType::BITMAP) {
            tex.bitmap = Image::FromBitmap(tex.filePath);
        }

        idxFromTextureName[name] = i;
        scene.textures.push_back(std::move(tex));
    }
}

void CRTSceneIO::parseMaterials(const json& j, Scene& scene, const std::map<std::string, size_t>& idxFromTextureName) {
    warnIfMissing(j, "materials");
    if (!j.contains("materials")) {
        scene.materials.push_back(Material{});
        std::cerr << "Substituting default material\n";
    }

    for (auto& jMaterial : j.at("materials")) {
        scene.materials.emplace_back();
        Material& material = scene.materials.back();
        material.smoothShading = Utils::jsonGetDefault<bool>(jMaterial, "smooth_shading", false);
        material.type = Material::TypeFromString(jMaterial.at("type"));
        material.occludes = Utils::jsonGetDefault<bool>(jMaterial, "occludes", true);
        material.ior = Utils::jsonGetDefault<float>(jMaterial, "ior", 1.f);
        material.setAlbedo({ 1.f, 1.f, 1.f });
        if (jMaterial.contains("albedo")) {
            if (jMaterial.at("albedo").is_string()) {
                std::string textureName = jMaterial.at("albedo").get<std::string>();
                material.textureIdx = idxFromTextureName.at(textureName);
                material.hasTexture = true;
            }
            else if (jMaterial.at("albedo").is_array()) {
                material.setAlbedo(Vec3FromJson(jMaterial.at("albedo")));
                material.hasTexture = false;
            }
            else { throw std::runtime_error("Error loading albedo: albedo is not a string or array\n"); }
        }
        if (material.type == Material::Type::REFRACTIVE) {
            // transparency + reflectivity + diffuseness = 1.f
            material.transparency = 0.9f;
            material.reflectivity = 0.1f;
        }

        else if (material.type == Material::Type::REFLECTIVE) {
            material.reflectivity = .8f;
            material.transparency = .0f;
        }

        material.diffuseness = 1 - material.transparency - material.reflectivity;
    }
}

Vec3 CRTSceneIO::Vec3FromJson(const json& j) {
    return Vec3{ j[0], j[1], j[2] };
}

void CRTSceneIO::parseObjects(const json& j, Scene& scene) {
    const auto& jObjects = j.at("objects");
    for (const auto& jObj : jObjects) {
        std::vector<Vec3> vertices;
        std::vector<Triangle> triangles;
        std::vector<Vec3> uvs;

        parseVertices(jObj, vertices);

        size_t materialIdx = jObj.at("material_index");
        if (scene.materials.size() <= materialIdx) {
            throw std::runtime_error("Error loading object: material index out of bounds");
        }
        parseTriangles(jObj, materialIdx, triangles);

        parseUvs(jObj, vertices.size(), uvs);

        assert(triangles.size() > 0);
        scene.addObject(vertices, triangles, uvs);
    }
}

void CRTSceneIO::parseVertices(const json& jObj, std::vector<Vec3>& vertices) {
    const auto& jVertices = jObj.at("vertices");

    if (!jVertices.is_array() || jVertices.size() % 3 != 0) {
        throw std::runtime_error("Error loading vertices: vertices is not an array or not divisible by 3");
    }

    for (size_t i = 0; i < jVertices.size(); i += 3) {
        float x = jVertices[i].get<float>();
        float y = jVertices[i + 1].get<float>();
        float z = jVertices[i + 2].get<float>();
        vertices.emplace_back(x, y, z);
    }
}

void CRTSceneIO::parseUvs(const json& jObj, const size_t expectedSize, std::vector<Vec3>& uvs)
{
    if (jObj.contains("uvs")) {
        const auto& jUvs = jObj.at("uvs");

        for (size_t i = 0; i < jUvs.size(); i += 3) {
            float u = jUvs[i].get<float>();
            float v = jUvs[i + 1].get<float>();
            float w = jUvs[i + 2].get<float>();
            uvs.emplace_back(u, v, w);
        }
    }
    else {
        uvs.resize(expectedSize, Vec3{ 0.f, 0.f, 0.f });
    }

    assert(uvs.size() == expectedSize);
}

void CRTSceneIO::parseTriangles(const json& jObj, const size_t materialIdx, std::vector<Triangle>& triangles) {

    const auto& jTriangles = jObj.at("triangles");
    for (size_t i = 0; i < jTriangles.size(); i += 3) {
        triangles.emplace_back(jTriangles[i], jTriangles[i + 1], jTriangles[i + 2], materialIdx);
    }
}

void CRTSceneIO::warnIfMissing(const json& j, const std::string& key) {
    if (!j.contains(key)) {
        std::cerr << "CRTSceneIO::warnIfMissing: key " << key << " not found\n";
    }
}

void CRTSceneIO::debugPrintNormals(const Scene& scene)
{
    std::cout << "Vertex Normals:\n";
    for (size_t i = 0; i < scene.cacheVertexNormals.size(); ++i) {
        const auto& vertNormal = scene.cacheVertexNormals[i];
        std::cout << i << ": " << vertNormal << '\n';
    }

    std::cout << "Triangle Normals:\n";
    for (size_t i = 0; i < scene.triangles.size(); ++i) {
        const auto& tri = scene.triangles[i];
        std::cout << i << ": " << tri.getNormal() << '\n';
    }
}


template <typename T>
void CRTSceneIO::assignIfExists(const json& j, std::string key, T& out) {
    if (j.contains(key)) {
        out = j.at(key).get<T>();
    }
}

template <>
void CRTSceneIO::assignIfExists<Vec3>(const json& j, std::string key, Vec3& out) {
    if (j.contains(key)) {
        out = Vec3FromJson(j.at(key));
    }
}
