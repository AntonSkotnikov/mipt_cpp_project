#include "MapParser.hpp"

#include <algorithm>
#include <fstream>
#include <stdexcept>
#include <string>

namespace plague::ui {

namespace {

constexpr const char * lowMapDirs[] = {
    "Interface/Inc/Maps/LowMap",
    "Interface/Maps/Low",
    "../Interface/Inc/Maps/LowMap",
    "../Interface/Maps/Low"
};

std::size_t utf8SymbolLength(unsigned char ch) {
    if ((ch & 0b10000000) == 0) return 1;
    if ((ch & 0b11100000) == 0b11000000) return 2;
    if ((ch & 0b11110000) == 0b11100000) return 3;
    if ((ch & 0b11111000) == 0b11110000) return 4;
    return 1;
}

}

std::vector<SymbolOnScreen> parseMapFile(const std::filesystem::path & path) {
    std::ifstream input(path);
    if (!input) {
        throw std::runtime_error("Could not open map file: " + path.string());
    }

    std::vector<SymbolOnScreen> symbols;
    std::string line;
    int y = 0;

    while (std::getline(input, line)) {
        int x = 0;

        for (std::size_t i = 0; i < line.size();) {
            const unsigned char ch = static_cast<unsigned char>(line[i]);
            const std::size_t length = std::min(utf8SymbolLength(ch), line.size() - i);
            std::string symbol = line.substr(i, length);

            if (symbol != " " && symbol != "\t" && symbol != "\r") {
                symbols.push_back({y, x, std::move(symbol)});
            }

            i += length;
            x++;
        }

        y++;
    }

    return symbols;
}

std::vector<SymbolOnScreen> parseLowMapCountry(const std::string & countryName) {
    for (const char * dir : lowMapDirs) {
        const std::filesystem::path path = std::filesystem::path(dir) / countryName;
        if (std::filesystem::exists(path)) {
            return parseMapFile(path);
        }
    }

    return parseMapFile(std::filesystem::path(lowMapDirs[0]) / countryName);
}

}
