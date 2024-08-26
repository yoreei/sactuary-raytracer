#include "include/SanctScene.h"
#include <fstream>

#include <memory>
#include "json.hpp"

#include "include/Scene.h"
#include "include/Image.h"
#include "include/MeshObject.h"
#include "include/Triangle.h"
#include "include/Material.h"
#include "include/Texture.h"
#include "include/Settings.h"
#include "include/Utils.h"
#include "include/RendererOutput.h"

using json = nlohmann::json;


void SanctSceneIO::write(const Settings& settings, const Scene& scene)
{
    std::filesystem::create_directories(settings.getSceneOutput());

    Path path = settings.getSceneOutput() / scene.sceneName;
    path = path.replace_extension(".sanctscene");
    std::string pathStr = path.string();

    std::cout << "Writing Scene: " << pathStr << std::endl;
    std::ofstream file(pathStr);
    if (!file) { throw std::runtime_error("Failed to load CRTScene file: " + pathStr); }

    json j = toSanctScene(scene);
    file << j.dump(4);
}

json SanctSceneIO::toSanctScene(const Scene& scene)
{
    json j{};
    scene;
    j["vertices"] = scene.vertices;
    j["lights"] = scene.lights;
    j["meshObjects"] = scene.meshObjects;
    j["materials"] = scene.materials;
    j["textures"] = scene.textures;
    j["triangles"] = scene.triangles;
    j["uvs"] = scene.uvs;
    return j;
}

void SanctSceneIO::load(const Settings& settings, const std::filesystem::path& filePath, Scene& scene)
{
    // Create a clean scene:
    std::string sceneName = filePath.filename().string();
    scene = Scene{ sceneName, &settings };

    std::string filePathStr = filePath.string();
    std::ifstream file(filePathStr);
    if (!file) { throw std::runtime_error("Failed to load CRTScene file: " + filePathStr); }

    json j;
    file >> j;

    std::cout << "Loading Scene: " << filePathStr << std::endl;

	j.at("lights").get_to(scene.lights);
    j.at("meshObjects").get_to(scene.meshObjects);
    j.at("materials").get_to(scene.materials);
    j.at("textures").get_to(scene.textures);
    j.at("triangles").get_to(scene.triangles);
    j.at("uvs").get_to(scene.uvs);
    j.at("vertices").get_to(scene.vertices);

    scene.build();

    settings;
    scene;
    filePath;
}
