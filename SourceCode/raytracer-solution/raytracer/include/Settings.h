/*
* filename: Settings.h
*/

#pragma once
#include <string>
#include <vector>
#include <filesystem>

class ImageSettings {
public:
    size_t startX = 0;
    size_t endX = 0;
    size_t startY = 0;
    size_t endY = 0;
    size_t bucketSize = 20;
};

class Settings {
public:
    // Unique ID for each render iteration. Not read from file
    size_t settingsId = 0;

    // File paths
    std::string sceneLibraryDir = "sceneLibrary";
    std::string projectDir = "hw13";
    std::vector<std::string> targetScenes {"scene0.crtscene"};
    std::string outputDir = "out";
    std::string compareDir = "compare";
    bool pruneInvisible = false;

    // Filled in during Scene loading
    ImageSettings imageSettings{};

    // Rendering settings
    size_t maxDepth = 16;
    float bias = 0.001f;

    // Debug settings
    bool debugPixel = true;
    size_t debugPixelX = 0;
    size_t debugPixelY = 0;

    bool debugLight = false;
    bool debugAccelStructure = false;
    bool showAabbs = false;

    // Resolution settings
    bool overrideResolution = false;
    size_t resolutionX = 300;
    size_t resolutionY = 200;

    // Output settings. TODO: add to parse
    bool bWritePng = true;
    bool bWriteBmp = true;

    // Optimization Settings
    bool forceNoAccelStructure = false;
    bool forceSingleThreaded = false;
    size_t maxTrianglesPerLeaf = 4;
    size_t accelTreeMaxDepth = 12345;

    /* @brief: Caller needs to catch exceptions from nlohmann::json. Missing keys is also an exception */
    static Settings load(const std::string& filename = "settings.json");

    /* Format back to Json-formatted string */
    std::string toString() const;

    void checkSettings() const;
    size_t debugPixelIdx(size_t imageWidth) const;
    std::string projectPath() const;
    std::string iterationName() const;
    std::string iterationPathNoExt() const;
    std::string framePathNoExt(const std::string& sceneName, size_t frameNumber) const;
    bool loadEntireProject() const;
    std::vector<std::filesystem::path> getOutputFiles() const;
    std::filesystem::path getCompareFile(std::filesystem::path file) const;
};