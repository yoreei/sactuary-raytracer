/* Main entry for Windows console. Reads settings and starts the engine */
#include <iostream>
#include <fstream>
#include <filesystem>

#include "include/Engine.h"
#include "include/Settings.h"

/* @brief Output benchmarking information for a settings permutation */
void logSettingsIteration(const Settings& settings) {
    std::ofstream logFile;
    logFile.open("out/" + settings.iterationName() + ".json", std::ios::out);
    logFile << settings.toString() << std::endl;
    std::cout << settings.toString() << std::endl;
    logFile.close();
}

/* @brief Add extra parameters */
void addBenchmarkingPermutations(std::vector<Settings>& settingsList)
{
    settingsList.back().accelTreeMaxDepth = 2;
    settingsList.back().maxTrianglesPerLeaf = 2;

    for (int i = 1; i < 30; ++i) {
        for (int j = 0; j < 30; ++j)
        {
            settingsList.push_back(settingsList.back());
            settingsList.back().settingsId += 1;
            settingsList.back().maxTrianglesPerLeaf += 2;
        }
        settingsList.back().accelTreeMaxDepth += 2;
        settingsList.back().maxTrianglesPerLeaf = 2;
    }
}


void compare(const Settings& settings)
{
    namespace fs = std::filesystem;
    using path = fs::path;
    std::cout << "-- Comparing Output --\n";

    std::vector<path> outputFiles = settings.getOutputFiles();
    for (const path& outputFile : outputFiles) {
        path compareFile = settings.getCompareFile(outputFile);
        if (fs::exists(compareFile)) {
            std::vector<Vec2<size_t>> diffs = Engine::diffImages(outputFile, compareFile);

			bool match = true;
            std::cout << "comparing " << outputFile << " and " << compareFile << std::endl;
			for (const auto& diff : diffs) {
				std::cout << "found diff: " << diff << std::endl;
				match = false;
			}
			if (match) {
                std::cout << "both files match\n";
			}
        }
        else {
            std::cout << outputFile << " has no corresponding compare file: " << compareFile.string() << "; skipping" << std::endl;
        }
    }
}

/* @brief Read settings and launch engine */
int launch()
{
    std::vector<Settings> settingsList = { Settings::load("settings.json") };

#ifdef NDEBUG
    if (settingsList.front().debugPixel) {
        throw std::runtime_error("Turn on Debug to debug pixels");
    }
#endif

    // addBenchmarkingPermutations(settingsList); // uncomment for benchmarking

    for (Settings& settings : settingsList) {
        std::cout << "Running iteration " << settings.iterationName() << std::endl;
        logSettingsIteration(settings);
        Engine engine{ settings };
		engine.runAllScenes();
		compare(settings);
    }


    if (settingsList.size() > 1) {
        std::cout << "\n\n\nGBestSettings:\n\n\n";
        std::cout << GBestSettings << std::endl;
    }

    return 0;
}

/* @brief As a console application, propagates all exceptions */
int main()
{
    // Loop main to detect race conditions, memory leaks, etc.
    size_t runTimes = 1;
    for (size_t i = 0; i < runTimes; ++i) {
        launch();
    }

}

