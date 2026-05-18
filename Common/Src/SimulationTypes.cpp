#include "SimulationTypes.hpp"

namespace plague {

// ========== РЕАЛИЗАЦИЯ МЕТОДОВ СИМУЛЯЦИОННЫХ ТИПОВ ==========

// CountryParams
double CountryParams::getScienceFactor() const {
    return medicine / 5.0;
}

double CountryParams::getVaccineSpeed() const {
    return 0.5 + (medicine / 5.0) * 0.5;
}

double CountryParams::getTransportFactor() const {
    return 0.2 + (urbanization / 5.0) * 0.8;
}

double CountryParams::getBorderCloseSpeed() const {
    return governmentReaction / 5.0;
}

// Population
Population::Population(double total)
    : initial(total), susceptible(total), exposed(0),
      infected(0), recovered(0), dead(0) {}

double Population::alive() const {
    return susceptible + exposed + infected + recovered;
}

// Country
Country::Country() : borderOpenness(1.0), bordersClosed(false) {}

// Virus
double Virus::getClimateModifier(int climateLevel) const {
    if (climateLevel < 1 || climateLevel > 5) return 1.0;
    return climateModifiers[climateLevel - 1];
}

// Humanity
double Humanity::getGlobalScientists() const {
    return scientistCommitment * 10000.0;
}

double Humanity::getBorderCloseThreshold() const {
    return 0.3 - awareness * 0.2;
}

// Vaccine
Vaccine::Vaccine() : progress(0), spreadRate(0.005), isReady(false), efficacy(0.95) {}

void Vaccine::updateProgress(double delta) {
    progress += delta;
    if (progress >= 100.0) {
        progress = 100.0;
        isReady = true;
    }
}

// CountryConnection
CountryConnection::CountryConnection(size_t f, size_t t, double volume)
    : from(f), to(t), transportVolume(volume), isActive(true) {}

// World methods
int World::getCountryIndex(const std::string& name) const {
    for (size_t i = 0; i < countries.size(); ++i) {
        if (countries[i].name == name) return static_cast<int>(i);
    }
    return -1;
}

void World::closeBorder(size_t i, size_t j) {
    for (auto& conn : connections) {
        if ((conn.from == i && conn.to == j) ||
            (conn.from == j && conn.to == i)) {
            conn.isActive = false;
        }
    }
    if (i < countries.size()) countries[i].bordersClosed = true;
    if (j < countries.size()) countries[j].bordersClosed = true;
}

void World::closeAllBorders(size_t countryIdx) {
    if (countryIdx >= countries.size()) return;
    countries[countryIdx].bordersClosed = true;
    countries[countryIdx].borderOpenness = 0.0;

    for (auto& conn : connections) {
        if (conn.from == countryIdx || conn.to == countryIdx) {
            conn.isActive = false;
        }
    }
}

void World::checkBorderClosures() {
    double threshold = humanity.getBorderCloseThreshold();

    for (size_t i = 0; i < countries.size(); ++i) {
        Country& country = countries[i];

        double infectionRate = country.pop.infected / country.pop.initial;
        double globalAwareness = humanity.awareness;

        double closeProbability = (infectionRate + globalAwareness) *
                                 country.params.getBorderCloseSpeed();

        if (closeProbability > threshold && !country.bordersClosed) {
            closeAllBorders(i);
        }
    }
}

} // namespace plague