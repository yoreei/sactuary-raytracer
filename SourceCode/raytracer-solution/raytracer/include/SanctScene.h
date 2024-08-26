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
class SanctSceneIO
{
public:
    static void write(const Settings& settings, const Scene& scene);
    static json toSanctScene(const Scene& scene);
    static void load(const Settings& settings, const std::filesystem::path& filePath, Scene& scene);
};
