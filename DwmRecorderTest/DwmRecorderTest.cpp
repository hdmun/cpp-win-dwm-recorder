#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <string>
#include <iostream>

#include "cxxopts.hpp"

int main(int argc, char* argv[])
{
    cxxopts::Options options("DwmRecorderTest");
    options.allow_unrecognised_options()
        .add_options()
        ("out", "recording output filename", cxxopts::value<std::string>())
        ("class", "record process class name", cxxopts::value<std::string>())
        ("window", "record process window name", cxxopts::value<std::string>())
        ;

    std::string outFileName;
    std::string className;
    std::string windowName;
    try {
        auto result = options.parse(argc, argv);
        if (!result.count("out")) {
            return 1;
        }

        if (!result.count("class")) {
            return 1;
        }

        if (!result.count("window")) {
            return 1;
        }

        outFileName = result["out"].as<std::string>();
        className = result["class"].as<std::string>();
        windowName = result["window"].as<std::string>();
    }
    catch (...) {
        std::cout << "Exception option parse" << std::endl;
        return 1;
    }

    HWND hFind = FindWindowA(className.c_str(), windowName.c_str());
    if (hFind == INVALID_HANDLE_VALUE) {
        std::cout << "Not FindWindow: " << className << ", " << windowName << std::endl;
        return 1;
    }

    std::cout << "DwmRecorderTest" << std::endl;

    return 0;
}
