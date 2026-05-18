#pragma once

#include "SimulationTypes.hpp"

#include <vector>

namespace plague {

std::vector<CountryConnection> buildInitialConnections(const std::vector<Country>& countries);

World initializeWorld();

void updateVaccineProgress(World& world);
void updateAwareness(World& world);
void propagateBetweenCountries(World& world);
void updateEpidemicModel(World& world);
void simulateDay(World& world);

}  // namespace plague
