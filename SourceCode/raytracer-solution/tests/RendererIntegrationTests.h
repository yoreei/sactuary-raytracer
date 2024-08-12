#pragma once

#include <filesystem>

#include "include/Renderer.h"
#include "include/Scene.h"
#include "include/UnitTestData.h"
#include "include/Animation.h"
#include "include/Animator.h"

namespace fs = std::filesystem;

namespace RendererIntegrationTests {

    std::string loadFile(const std::string& path) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string str = buffer.str();
            return str;
        }
       else {
            throw std::runtime_error("Could not open file: " + path);
        }
    }

    void run() {
        Scene scene{"test"};
        Image image{300, 200};
        Animator animator {scene, 0};

        using SceneFunc = std::function<void(Scene&, Animator&)>;
        std::vector<SceneFunc> sceneLoaders = {};
        for (auto& sceneLoader : sceneLoaders) {
            sceneLoader(scene, animator);

            Renderer renderer{};
            renderer.renderScene(scene, image);

            if (fs::exists("ref/")) {
                std::string sRef = loadFile("ref/" + scene.sceneName+ ".ppm");
                std::string sOut = image.toPpmString();
                assert(sRef == sOut);
            }
            else {
                std::cout << "Reference image directory not found. Generating new references." << std::endl;
                fs::create_directories("ref/");
                image.writeToPpm("ref/" + scene.sceneName + ".ppm");
            }
        }
    }
} // namespace RendererUnitTests