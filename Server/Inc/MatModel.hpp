#pragma once

#include "SimulationTypes.hpp"
#include <vector>
#include <string>

namespace plague {

void updateVaccineProgress(World& world);
void updateAwareness(World& world);
void propagateBetweenCountries(World& world);
void updateEpidemicModel(World& world);
void simulateDay(World& world);

World initializeWorld();

} 