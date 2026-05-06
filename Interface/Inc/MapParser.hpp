#pragma once

#include "Widget.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace plague::ui {

std::vector<SymbolOnScreen> parseMapFile(const std::filesystem::path & path);
std::vector<SymbolOnScreen> parseLowMapCountry(const std::string & countryName);

}
