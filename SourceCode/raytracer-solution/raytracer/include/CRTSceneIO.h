#pragma once
#include <string>
#include <vector>
#include <filesystem>

#include "json.hpp"

class Settings;
class Triangle;
class Image;
class Scene;
class Vec3;

using json = nlohmann::json;

class RendererOutput;
class CRTSceneIO
{
public:
    static void write(const Settings& settings, const Scene& scene);
    static void load(const Settings& settings, const std::filesystem::path& filePath, Scene& scene);
private:
    static void parseLight(const json& j, Scene& scene);
    static void parseSettings(const json& j, Scene& scene, const Settings& settings);
    static void parseImageSettings(const json& j, Scene& scene, const Settings& settings);
    static void parseCameraSettings(const json& j, Scene& scene);
    static void parseTextures(const json& j, Scene& scene, const Settings& settings, std::map<std::string, size_t>& idxFromTextureName);
    static void parseMaterials(const json& j, Scene& scene, const std::map<std::string, size_t>& idxFromTextureName);
    static void parseObjects(const json& j, Scene& scene);
    static void parseVertices(const json& jObj, std::vector<Vec3>& vertices);
    static void parseUvs(const json& jObj, const size_t expectedSize, std::vector<Vec3>& uvs);
    static void parseTriangles(const json& jObj, const size_t materialIdx, std::vector<Triangle>& triangles);
    static Vec3 Vec3FromJson(const json& j);
    static void warnIfMissing(const json& j, const std::string& key);
    /* @brief Assigns value to `out` if key exists, otherwise does nothing */
    template <typename T>
    static void assignIfExists(const json& j, std::string key, T& out);
    template <>
    void assignIfExists<Vec3>(const json& j, std::string key, Vec3& out);
    /* Handy for debugging normals generation */
    static void debugPrintNormals(const Scene& scene);
};

