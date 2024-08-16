// Settings.cpp

#include "include/Settings.h"
#include <fstream>
#include <string>
#include "json.hpp"

Settings Settings::load(const std::string& filename) {
    Settings settings;
    std::ifstream file(filename);
    nlohmann::json json;
    file >> json;

    settings.sceneLibraryDir = json.at("sceneLibraryDir");
    settings.projectDir = json.at("projectDir");
    settings.targetScenes = json.at("targetScenes").get<std::vector<std::string>>();
    settings.outputDir = json.at("outputDir");
    settings.pruneInvisible = json.at("pruneInvisible");
    settings.maxDepth = json.at("maxDepth");
    settings.bias = json.at("bias");
    settings.debugPixel = json.at("debugPixel");
    settings.debugPixelX = json.at("debugPixelX");
    settings.debugPixelY = json.at("debugPixelY");
    settings.debugLight = json.at("debugLight");
    settings.debugAccelStructure = json.at("debugAccelStructure");
    settings.showAabbs = json.at("showAabbs");
    settings.overrideResolution = json.at("overrideResolution");
    settings.resolutionX = json.at("resolutionX");
    settings.resolutionY = json.at("resolutionY");
    settings.bWritePng = json.at("bWritePng");
    settings.bWriteBmp = json.at("bWriteBmp");
    settings.forceNoAccelStructure = json.at("forceNoAccelStructure");
    settings.forceSingleThreaded = json.at("forceSingleThreaded");
    settings.maxTrianglesPerLeaf = json.at("maxTrianglesPerLeaf");
    settings.accelTreeMaxDepth = json.at("accelTreeMaxDepth");

    settings.checkSettings();

    return settings;
}

std::string Settings::toString() const {
    nlohmann::json json;

    json["sceneLibraryDir"] = sceneLibraryDir;
    json["projectDir"] = projectDir;
    json["targetScenes"] = targetScenes;
    json["outputDir"] = outputDir;
    json["maxDepth"] = maxDepth;
    json["pruneInvisible"] = pruneInvisible;
    json["bias"] = bias;
    json["debugPixel"] = debugPixel;
    json["debugPixelX"] = debugPixelX;
    json["debugPixelY"] = debugPixelY;
    json["debugLight"] = debugLight;
    json["debugAccelStructure"] = debugAccelStructure;
    json["showAabbs"] = showAabbs;
    json["overrideResolution"] = overrideResolution;
    json["resolutionX"] = resolutionX;
    json["resolutionY"] = resolutionY;
    json["bWritePng"] = bWritePng;
    json["bWriteBmp"] = bWriteBmp;
    json["forceNoAccelStructure"] = forceNoAccelStructure;
    json["forceSingleThreaded"] = forceSingleThreaded;
    json["maxTrianglesPerLeaf"] = maxTrianglesPerLeaf;
    json["accelTreeMaxDepth"] = accelTreeMaxDepth;

    return json.dump();
}


std::string Settings::projectPath() const
{
    return outputDir + "/" + iterationName() + "/" + projectDir;
}

std::string Settings::iterationName() const
{
    return "Settings" + std::to_string(settingsId);
}

bool Settings::loadEntireProject() const
{
    return targetScenes.empty();
}

std::vector<std::filesystem::path> Settings::getOutputFiles() const
{
	std::vector<std::filesystem::path> pngFiles;

	for (const auto& entry : std::filesystem::recursive_directory_iterator(outputDir)) {
		if (entry.is_regular_file() && entry.path().extension() == ".png") {
			pngFiles.push_back(entry.path());
		}
	}

    return pngFiles;
}

std::filesystem::path Settings::getCompareFile(std::filesystem::path file) const
{
    std::filesystem::path relativePath;

    auto it = file.begin();

    if (file.has_root_path()) {
        ++it; // Skip root ("C:\" on Windows or "/" on Unix)
    }

    // Skip outputDir
    if (it != file.end()) {
        if ((*it).string() != outputDir) { throw std::runtime_error("unexpected path structure"); }
        ++it;
    }

    // Rebuild the remaining path
    for (; it != file.end(); ++it) {
        relativePath /= *it;
    }

    std::filesystem::path compareFilePath = compareDir / relativePath;

    return compareFilePath;
}

std::string Settings::iterationPathNoExt() const {
    return outputDir + "/" + iterationName();
}
std::string Settings::framePathNoExt(const std::string& sceneName, size_t frameNumber) const {
    return outputDir + "/" + iterationName() + "/" + projectDir + "/" + sceneName + "/" + std::to_string(frameNumber);
}

size_t Settings::debugPixelIdx(size_t imageWidth) const {
    return debugPixelY * imageWidth + debugPixelX;
}

void Settings::checkSettings() const {
    if (debugPixel && overrideResolution) {
        if (debugPixelX > resolutionX || debugPixelY > resolutionY) {
            throw std::runtime_error("no such pixel to debug");
        }
    }
}
