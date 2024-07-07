#include <fstream>

#include "json.hpp"

#include "CRTSceneLoader.h"
#include "Scene.h"
#include "Image.h"
#include "Material.h"

using json = nlohmann::json;

[[nodiscard]]
bool CRTSceneLoader::loadCrtscene(const std::string& filename, Scene& scene, Image& image) {
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Unable to open file: " << filename << '\n';
        return false;
    }

    json j;
    try {
        file >> j;
    }
    catch (const json::exception& e) {
        std::cerr << "Error parsing JSON file: " << e.what() << '\n';
        return false;
    }

    if (!validateCrtscene(j) ) {
        return false;
    }

    if (!parseBackgroundColor(j, scene) ||
        !parseImageSettings(j, image) ||
        !parseCameraSettings(j, scene) ||
        !parseMaterials(j, scene) ||
        !parseObjects(j, scene) ||
        !parseLight(j, scene)) {
        return false;
    }
    genVertexNormals(scene);
    return true;
}

bool CRTSceneLoader::validateCrtscene(const json& j) {
    // TODO: move more validation to this function
    for (const auto& element : j.items()) {
        const std::string& key = element.key();
        const auto& val = element.value();
        if (key == "settings") {
            if (!val.contains("background_color") || !val.contains("image_settings")) {
                std::cerr << "Error loading settings: background_color or image_settings not found\n";
                return false;
            }
        }
        else if (key == "camera") {
            if (!val.contains("position") || !val.contains("matrix")) {
                std::cerr << "Error loading camera: position or matrix not found\n";
                return false;
            }
        }
        else if (key == "materials") {
            if (!val.is_array() && val.size() > 0) {
                std::cerr << "Error loading materials: materials is not an array\n";
                return false;
            }
            for (const auto& jMaterial : val) {
                if (!jMaterial.contains("type") || !jMaterial.contains("albedo") || !jMaterial.contains("smooth_shading")) {
                    std::cerr << "Error loading material: type, albedo or smooth_shading not found\n";
                    return false;
                }
            }
        }
        else if (key == "objects") {
            if (!val.is_array() && val.size() > 0) {
                std::cerr << "Error loading objects: objects is not an array\n";
                return false;
            }
            for (const auto& jObj : val) {
                if (!jObj.contains("vertices") || !jObj.contains("triangles") || !jObj.contains("material_index")) {
                    std::cerr << "Error loading object: vertices or triangles or material_index not found\n";
                    return false;
                }
            }
        }
        else if (key == "lights") {
            if (!val.is_array()) {
                std::cerr << "Error loading lights: lights is not an array\n";
                return false;
            }
        }
        else {
            std::cerr << "Error loading CRTScene: unknown key: " << key << '\n';
            return false;
        }
    }
    return true;
}

bool CRTSceneLoader::parseLight(const json& j, Scene& scene)
{
    if (!j.contains("lights") || j.at("lights").empty()) {
        std::cerr << "Heads up: no lights found!\n";
        return true;
    }
    else if (!j.at("lights").is_array()) {
        std::cerr << "Error loading lights: lights is not an array\n";
        return false;
    }
    else {
        for (const auto& jLight : j.at("lights")) {
            if (!jLight.contains("intensity") || !jLight.contains("position")) {
                std::cerr << "Error loading light: intensity or position not found\n";
                return false;
            }

            if (!jLight.at("position").is_array() || jLight.at("position").size() != 3) {
                std::cerr << "Error loading light: position is not an array or not of size 3\n";
                return false;
            }

            float intensity = jLight.at("intensity");
            Vec3 pos{ jLight.at("position")[0], jLight.at("position")[1], jLight.at("position")[2] };
            scene.lights.emplace_back(pos, intensity, Color{0, 0, 0});
        }
    }
    return true;
}

inline bool CRTSceneLoader::parseBackgroundColor(const json& j, Scene& scene) {
    try {
        const auto& jBgColor = j.at("settings").at("background_color");

        if (!jBgColor.is_array() || jBgColor.size() != 3) {
            std::cerr << "Error loading background_color: background_color is not an array or not of size 3\n";
            return false;
        }

        scene.bgColor = Vec3{ jBgColor[0], jBgColor[1], jBgColor[2] };
        return true;
    }
    catch (const json::exception& e) {
        std::cerr << "Error loading background_color: " << e.what() << '\n';
        return false;
    }
}

inline bool CRTSceneLoader::parseImageSettings(const json& j, Image& image) {
    try {
        const auto& jImgSettings = j.at("settings").at("image_settings");
        image = Image(jImgSettings.at("width"), jImgSettings.at("height"));
        return true;
    }
    catch (const json::exception& e) {
        std::cerr << "Error loading image_settings: " << e.what() << '\n';
        return false;
    }
}

inline bool CRTSceneLoader::parseCameraSettings(const json& j, Scene& scene) {
    try {
        const auto& jCamPos = j.at("camera").at("position");
        Vec3 camPos{ jCamPos.at(0), jCamPos.at(1), jCamPos.at(2) };

        const auto& jRotateMat = j.at("camera").at("matrix");
        Matrix3x3 rotateMat{ jRotateMat.get<std::vector<float>>() };
        // NB: CRTScene format gives us the inverse direction of the camera.
        Matrix3x3 camMat = Camera::DefaultMatrix * rotateMat;        

        scene.camera = Camera{ 90.f, camPos, camMat };
        return true;
    }
    catch (const json::exception& e) {
        std::cerr << "Error loading camera settings: " << e.what() << '\n';
        return false;
    }
}

inline bool CRTSceneLoader::parseMaterials(const json& j, Scene& scene) {
    for (auto& jMaterial : j.at("materials")) {
        auto type = Material::TypeFromString(jMaterial.at("type"));
        Vec3 albedo = Vec3FromJson(jMaterial.at("albedo"));
        bool smoothShading = boolFromJson(jMaterial.at("smooth_shading"));
        scene.materials.emplace_back(albedo, smoothShading, type);
    }

    return true;

    /*
        "materials": [{
                "type": "diffuse",
                "albedo": [
                    0, 1, 1
                ],
                "smooth_shading": false
            }
        ],
    */

}

inline Vec3 CRTSceneLoader::Vec3FromJson(const json& j) {
    if (!j.is_array() || j.size() != 3) {
        std::cerr << "Error loading Vec3: Vec3 is not an array or not of size 3\n";
        return Vec3{ };
    }
    return Vec3{ j[0], j[1], j[2] };
}

inline bool CRTSceneLoader::boolFromJson(const json& j) {
    if (!j.is_boolean()) {
        std::cerr << "Error loading bool: not a boolean\n";
        return false;
    }
    return j.get<bool>();
}

inline bool CRTSceneLoader::parseObjects(const json& j, Scene& scene) {
    std::vector<Scene> scenes {};

    try {
        const auto& jObjects = j.at("objects");
        for (const auto& jObj : jObjects) {
            Scene objScene{};

            if (!parseVertices(jObj, objScene)) {
                return false;
            }
            if (!parseTriangles(jObj, objScene)) {
                return false;
            }

            if (!objScene.bakeObject()) {
                return false;
            }

            scenes.push_back(objScene);
        }

    }
    catch (const json::exception& e) {
        std::cerr << "Error loading objects: " << e.what() << '\n';
        return false;
    }
    scene.addObjects(scenes);
    return true;
}

inline bool CRTSceneLoader::parseVertices(const json& jObj, Scene& scene) {
    auto& vertices = scene.vertices;
    try {
        const auto& jVertices = jObj.at("vertices");

        if (!jVertices.is_array() || jVertices.size() % 3 != 0) {
            std::cerr << "Error loading vertices: vertices is not an array or not divisible by 3\n";
            return false;
        }

        for (size_t i = 0; i < jVertices.size(); i += 3) {
            float y = static_cast<float>(jVertices[i + 1]);
            vertices.emplace_back(jVertices[i], y, jVertices[i + 2]);
        }
    }
    catch (const json::exception& e) {
        std::cerr << "Error loading vertices: " << e.what() << '\n';
        return false;
    }

    return true;
}

inline bool CRTSceneLoader::parseTriangles(const json& jObj, Scene& scene) {
    auto& triangles = scene.triangles;
    size_t materialIndex = jObj.at("material_index");
    try {
        const auto& jTriangles = jObj.at("triangles");

        if (!jTriangles.is_array() || jTriangles.size() % 3 != 0) {
            std::cerr << "Error loading triangles: triangles is not an array or not divisible by 3\n";
            return false;
        }

        for (size_t i = 0; i < jTriangles.size(); i += 3) {
            triangles.emplace_back(scene.vertices, jTriangles[i], jTriangles[i + 1], jTriangles[i + 2], materialIndex);
        }
    }
    catch (const json::exception& e) {
        std::cerr << "Error loading triangles: " << e.what() << '\n';
        return false;
    }

    return true;
}

void CRTSceneLoader::genVertexNormals(Scene& scene) {
    // Vec3{0.f, 0.f, 0.f} is important for summation
    scene.vertexNormals.resize(scene.vertices.size(), Vec3{0.f, 0.f, 0.f});
    for(size_t i = 0; i < scene.vertices.size(); ++i) {
        std::vector<size_t> attachedTriangles {};
        genAttachedTriangles(scene, i, attachedTriangles);
        for(size_t triIndex : attachedTriangles) {
            scene.vertexNormals[i] += scene.triangles[triIndex].getNormal();
        }
    }

    for(Vec3& normal : scene.vertexNormals) {
        normal.normalize();
    }
}

/**
 * @brief Generate a list of triangle indexes that are attached to a vertex
 * 
 * @param scene Scene object
 * @param vertexIndex Index of the vertex
 * @param attachedTriangles Output list of triangle indexes
 */
void CRTSceneLoader::genAttachedTriangles(const Scene& scene, size_t vertexIndex, std::vector<size_t>& attachedTriangles) {
    for(size_t i = 0; i < scene.triangles.size(); ++i) {
        const Triangle& tri = scene.triangles[i];
        if(tri.hasVertex(vertexIndex)) {
            attachedTriangles.push_back(i);
        }
    }
}