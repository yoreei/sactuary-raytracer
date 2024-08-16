#pragma warning( disable : 4365 )

#include <fstream>
#include <cstdint>
#include <memory>
#include <string>
#include <sstream>
#include <filesystem>

#include "json.hpp"

#include "include/Engine.h"
#include "include/Image.h"
#include "include/CRTTypes.h"
#include "include/Triangle.h"
#include "include/Camera.h"
#include "include/Scene.h"
#include "include/Metrics.h"
#include "include/CRTSceneIO.h"
#include "include/Renderer.h"
#include "include/Settings.h"
#include "include/Scripts.h"
#include "include/Globals.h"
#include "include/Utils.h"

namespace fs = std::filesystem;

void Engine::writeFile(const std::string& filename, const std::string& data) const {
    std::ofstream ppmFileStream(filename, std::ios::out | std::ios::binary);
    ppmFileStream.write(data.c_str(), data.size());
    ppmFileStream.close();
}

void Engine::handleOverrideSettings()
{
    if (settings.debugPixel) {
        rendererOutput.startX = settings.debugPixelX;
        rendererOutput.startY = settings.debugPixelY;
        rendererOutput.endX = settings.debugPixelX + 1;
        rendererOutput.endY = settings.debugPixelY + 1;
    }
}

void Engine::startTick()
{
    std::string directory = settings.projectPath() + "/" + scene.sceneName;
    fs::create_directories(directory);

    while (GFrameNumber <= GEndFrame) {
        std::cout << ">>> Frame: " << GFrameNumber << std::endl;
        cleanFrame();

        Scripts::onTick(scene);
        scene.updateAnimations();
        renderer.render();
        writeFrame();
        ++GFrameNumber;
        auto summedMetrics = GSceneMetrics.toJson();

        uint64_t triInt = Utils::jsonGetDefault<uint64_t>(summedMetrics["counters"], "TriangleIntersection", 0);
        if (triInt < GBestTriangleIntersect) {
            GBestSettings = summedMetrics.dump(4);
        }
    }
}

std::vector<Vec2<size_t>> Engine::diffImages(std::filesystem::path path1, std::filesystem::path path2)
{
    Image img1 = Image::FromBitmap(path1.string());
    Image img2 = Image::FromBitmap(path2.string());
    return img1.diff(img2);
}

void Engine::loadScene(const std::filesystem::path& filePath) {
    std::cout << ">> Loading scene: " << filePath << std::endl;
	GSceneMetrics.clear();
    GResetGlobals();

    GSceneMetrics.startTimer("loadScene");

    CRTSceneIO::loadCrtscene(settings, filePath, scene, rendererOutput);
    handleOverrideSettings();
    rendererOutput.init();
    Scripts::onSceneLoaded(scene);
    std::cout << ">> Scene " << filePath << " loaded\n";

    GSceneMetrics.stopTimer("loadScene");
}

int Engine::runAllScenes()
{
    std::vector<std::filesystem::path> scenePaths = getScenesToLoad();

    for (const std::filesystem::path& scenePath : scenePaths) {
        loadScene(scenePath);
        startTick();
    }

    return 0;
}

std::vector<std::filesystem::path> Engine::getScenesToLoad() const
{
    using path = std::filesystem::path;
	std::vector<path> scenePaths{};
    using dir = std::filesystem::directory_entry;

    if (settings.loadEntireProject()) {
        for (const dir& entry : fs::directory_iterator(settings.sceneLibraryDir + "/" + settings.projectDir)) {
            const path scenePath = entry.path();
            if (path ext = scenePath.extension(); ext == ".crtscene") {
                scenePaths.push_back(scenePath);
            }
        }
    }
    else {
        for (const std::string& scenePath : settings.targetScenes) {
            path scenePathHelper = path(settings.sceneLibraryDir + "/" + settings.projectDir + "/" + scenePath);
            scenePaths.push_back(scenePathHelper);
        }
    }
    return scenePaths;
}

void Engine::writeFrame() const {
    // Write Flat Image
    Image image = rendererOutput.getFlatImage();
    std::string framePathNoExt = settings.framePathNoExt(scene.sceneName, GFrameNumber);
    image.writeImage(framePathNoExt, settings.bWritePng, settings.bWriteBmp);

    // Write Depth Images
    std::vector<Image> depthImages = rendererOutput.getDepthImages();
    for (size_t i = 0; i < depthImages.size(); i++) {
        depthImages[i].writeImage(framePathNoExt + "_depth_" + std::to_string(i), settings.bWritePng, settings.bWriteBmp);
    }

    // Write Text Logs
    std::ofstream fileStream(framePathNoExt + ".log", std::ios::out);
    std::string metricsString = GSceneMetrics.toString();
    std::stringstream stream;

    stream << metricsString << std::endl;
    stream << std::endl;

    if (settings.debugPixel) {
        stream << "debugPixel: " << image(settings.debugPixelX, settings.debugPixelY) << std::endl;
    }

    std::string logStr = stream.str();
    std::cout << logStr;
    fileStream << logStr;

    // Write Output Scene

}


void Engine::cleanFrame()
{
    rendererOutput.init();
    GSceneMetrics.clear();
}

