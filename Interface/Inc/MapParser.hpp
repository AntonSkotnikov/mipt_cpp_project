#pragma once

#include "Widget.hpp"
#include "Settings.hpp"
#include <filesystem>
#include <string>
#include <vector>

namespace plague::ui {

/**
 * @brief Parse a country map asset into drawable screen symbols.
 * @param path Path to a text map file.
 * @return Non-space symbols with their country-local coordinates.
 */
std::vector<SymbolOnScreen> parseMapFile(const std::filesystem::path & path);

/**
 * @brief Parse a country map using the low-resolution map set.
 * @param countryName Display/server country name.
 * @return Non-space symbols with their country-local coordinates.
 */
std::vector<SymbolOnScreen> parseLowMapCountry(const std::string & countryName);

/**
 * @brief Parse a country map for the requested UI resolution.
 * @param countryName Display/server country name.
 * @param resolution Current UI resolution profile.
 * @return Non-space symbols with their country-local coordinates.
 */
std::vector<SymbolOnScreen> parseMapCountry(const std::string & countryName, Resolutions resolution);

}
