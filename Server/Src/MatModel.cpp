#include "SimulationTypes.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <random>

namespace plague {

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ИНИЦИАЛИЗАЦИИ ==========

// ---------- Инициализация связей (с использованием CountryConnection) ----------
std::vector<CountryConnection> buildInitialConnections(const std::vector<Country>& countries) {
    std::vector<CountryConnection> connections;
    auto idx = [&](const std::string& name) -> int {
        for (size_t i = 0; i < countries.size(); ++i)
            if (countries[i].name == name) return (int)i;
        return -1;
    };
    auto connect = [&](const std::string& a, const std::string& b, double volume = 1000.0) {
        int ia = idx(a), ib = idx(b);
        if (ia >= 0 && ib >= 0) {
            connections.emplace_back(ia, ib, volume);
        }
    };

    connect("CHINA", "RUSSIA", 5000.0);
    connect("CHINA", "INDIA", 8000.0);
    connect("CHINA", "USA", 3000.0);
    connect("CHINA", "JAPAN", 3500.0);
    connect("CHINA", "MONGOLIA", 1200.0);
    connect("CHINA", "KAZAKHSTAN", 2000.0);
    connect("CHINA", "SOUTH EAST ASIA", 4500.0);
    connect("CHINA", "AUSTRALIA", 2500.0);

    connect("RUSSIA", "USA", 2000.0);
    connect("RUSSIA", "CHINA", 5000.0);
    connect("RUSSIA", "KAZAKHSTAN", 1600.0);
    connect("RUSSIA", "UKRAINE", 1500.0);
    connect("RUSSIA", "BELARUS", 1200.0);
    connect("RUSSIA", "SCANDINAVIA", 1800.0);
    connect("RUSSIA", "W EUROPE", 2500.0);
    connect("RUSSIA", "TURKEY", 2000.0);
    connect("RUSSIA", "JAPAN", 1500.0);

    connect("USA", "INDIA", 4000.0);
    connect("USA", "BRAZIL", 3500.0);
    connect("USA", "CHINA", 3000.0);
    connect("USA", "RUSSIA", 2000.0);
    connect("USA", "CANADA", 4500.0);
    connect("USA", "MEXICO", 5000.0);
    connect("USA", "UK", 3500.0);
    connect("USA", "W EUROPE", 4000.0);
    connect("USA", "JAPAN", 3000.0);
    connect("USA", "AUSTRALIA", 2500.0);
    connect("USA", "N SOUTH AMERICA", 2000.0);

    connect("INDIA", "BRAZIL", 2500.0);
    connect("INDIA", "CHINA", 8000.0);
    connect("INDIA", "USA", 4000.0);
    connect("INDIA", "MIDDLE EAST", 3500.0);
    connect("INDIA", "SOUTH EAST ASIA", 4000.0);
    connect("INDIA", "UK", 2500.0);
    connect("INDIA", "AUSTRALIA", 2000.0);

    connect("BRAZIL", "S AFRICA", 1500.0);
    connect("BRAZIL", "USA", 3500.0);
    connect("BRAZIL", "INDIA", 2500.0);
    connect("BRAZIL", "N SOUTH AMERICA", 1800.0);
    connect("BRAZIL", "SW SOUTH AMERICA", 1800.0);
    connect("BRAZIL", "W EUROPE", 2500.0);

    connect("S AFRICA", "BRAZIL", 1500.0);
    connect("S AFRICA", "CHINA", 2000.0);
    connect("S AFRICA", "M AFRICA", 1800.0);
    connect("S AFRICA", "MADAGASCAR", 700.0);
    connect("S AFRICA", "INDIA", 2500.0);
    connect("S AFRICA", "W EUROPE", 2000.0);
    connect("S AFRICA", "UK", 1800.0);

    connect("CANADA", "USA", 4500.0);
    connect("CANADA", "UK", 2000.0);
    connect("CANADA", "SCANDINAVIA", 1500.0);
    connect("CANADA", "RUSSIA", 1000.0);
    connect("CANADA", "GREENLAND", 800.0);

    connect("MEXICO", "USA", 5000.0);
    connect("MEXICO", "N SOUTH AMERICA", 2500.0);
    connect("MEXICO", "SW SOUTH AMERICA", 2000.0);
    connect("MEXICO", "W EUROPE", 1800.0);

    connect("UK", "W EUROPE", 4500.0);
    connect("UK", "ICELAND", 900.0);
    connect("UK", "SCANDINAVIA", 2000.0);
    connect("UK", "USA", 3500.0);
    connect("UK", "CANADA", 2000.0);
    connect("UK", "INDIA", 2500.0);
    connect("UK", "S AFRICA", 1800.0);
    connect("UK", "AUSTRALIA", 2500.0);
    connect("UK", "MIDDLE EAST", 2200.0);

    connect("W EUROPE", "SCANDINAVIA", 2500.0);
    connect("W EUROPE", "TURKEY", 3200.0);
    connect("W EUROPE", "UK", 4500.0);
    connect("W EUROPE", "RUSSIA", 2500.0);
    connect("W EUROPE", "USA", 4000.0);
    connect("W EUROPE", "MIDDLE EAST", 3000.0);
    connect("W EUROPE", "N AFRICA", 2500.0);
    connect("W EUROPE", "BRAZIL", 2500.0);
    connect("W EUROPE", "INDIA", 2800.0);
    connect("W EUROPE", "S AFRICA", 2000.0);
    connect("W EUROPE", "UKRAINE", 1800.0);

    connect("SCANDINAVIA", "W EUROPE", 2500.0);
    connect("SCANDINAVIA", "RUSSIA", 1800.0);
    connect("SCANDINAVIA", "UK", 2000.0);
    connect("SCANDINAVIA", "ICELAND", 1200.0);
    connect("SCANDINAVIA", "CANADA", 1500.0);
    
    connect("TURKEY", "MIDDLE EAST", 2800.0);
    connect("TURKEY", "W EUROPE", 3200.0);
    connect("TURKEY", "RUSSIA", 2000.0);
    connect("TURKEY", "UKRAINE", 1500.0);
    connect("TURKEY", "N AFRICA", 2000.0);
    connect("TURKEY", "INDIA", 2500.0);

    connect("MIDDLE EAST", "INDIA", 3500.0);
    connect("MIDDLE EAST", "N AFRICA", 2500.0);
    connect("MIDDLE EAST", "TURKEY", 2800.0);
    connect("MIDDLE EAST", "W EUROPE", 3000.0);
    connect("MIDDLE EAST", "SOUTH EAST ASIA", 2500.0);
    connect("MIDDLE EAST", "UK", 2200.0);

    connect("N AFRICA", "M AFRICA", 1800.0);
    connect("N AFRICA", "MIDDLE EAST", 2500.0);
    connect("N AFRICA", "W EUROPE", 2500.0);
    connect("N AFRICA", "TURKEY", 2000.0);
    connect("N AFRICA", "S AFRICA", 1500.0);

    connect("M AFRICA", "S AFRICA", 1800.0);
    connect("M AFRICA", "N AFRICA", 1800.0);
    
    connect("AUSTRALIA", "OCEANIA", 2200.0);
    connect("AUSTRALIA", "NEW ZELAND", 1800.0);
    connect("AUSTRALIA", "CHINA", 2500.0);
    connect("AUSTRALIA", "USA", 2500.0);
    connect("AUSTRALIA", "JAPAN", 2000.0);
    connect("AUSTRALIA", "INDIA", 2000.0);
    connect("AUSTRALIA", "UK", 2500.0);
    connect("AUSTRALIA", "SOUTH EAST ASIA", 3000.0);

    connect("NEW ZELAND", "AUSTRALIA", 1800.0);
    connect("NEW ZELAND", "OCEANIA", 1500.0);
    connect("NEW ZELAND", "USA", 1800.0);

    connect("OCEANIA", "AUSTRALIA", 2200.0);
    connect("OCEANIA", "NEW ZELAND", 1500.0);
    connect("OCEANIA", "SOUTH EAST ASIA", 2000.0);

    connect("JAPAN", "CHINA", 3500.0);
    connect("JAPAN", "RUSSIA", 1500.0);
    connect("JAPAN", "USA", 3000.0);
    connect("JAPAN", "AUSTRALIA", 2000.0);
    connect("JAPAN", "SOUTH EAST ASIA", 3000.0);

    connect("MONGOLIA", "CHINA", 1200.0);
    connect("MONGOLIA", "RUSSIA", 1000.0);

    connect("KAZAKHSTAN", "RUSSIA", 1600.0);
    connect("KAZAKHSTAN", "CHINA", 2000.0);
    connect("KAZAKHSTAN", "MIDDLE EAST", 1500.0);
    connect("KAZAKHSTAN", "INDIA", 1800.0);
    connect("UKRAINE", "RUSSIA", 1500.0);
    connect("UKRAINE", "BELARUS", 1200.0);
    connect("UKRAINE", "W EUROPE", 1800.0);
    connect("UKRAINE", "TURKEY", 1500.0);

    connect("BELARUS", "RUSSIA", 1200.0);
    connect("BELARUS", "UKRAINE", 1200.0);
    connect("BELARUS", "W EUROPE", 1500.0);
    connect("BELARUS", "SCANDINAVIA", 1000.0);

    connect("GREENLAND", "ICELAND", 800.0);
    connect("GREENLAND", "CANADA", 800.0);
    connect("GREENLAND", "SCANDINAVIA", 1000.0);

    connect("ICELAND", "UK", 900.0);
    connect("ICELAND", "SCANDINAVIA", 1200.0);
    connect("ICELAND", "GREENLAND", 800.0);
    connect("ICELAND", "CANADA", 1000.0);

    connect("N SOUTH AMERICA", "BRAZIL", 1800.0);
    connect("N SOUTH AMERICA", "SW SOUTH AMERICA", 1500.0);
    connect("N SOUTH AMERICA", "USA", 2000.0);
    connect("N SOUTH AMERICA", "MEXICO", 2500.0);
    connect("N SOUTH AMERICA", "W EUROPE", 2000.0);

    connect("SW SOUTH AMERICA", "BRAZIL", 1800.0);
    connect("SW SOUTH AMERICA", "N SOUTH AMERICA", 1500.0);
    connect("SW SOUTH AMERICA", "MEXICO", 2000.0);
    connect("SW SOUTH AMERICA", "W EUROPE", 1800.0);

    connect("MADAGASCAR", "S AFRICA", 700.0);
    connect("MADAGASCAR", "INDIA", 1500.0);

    return connections;
}

// ---------- Инициализация мира ----------
World initializeWorld() {
    World world;

    struct RawCountry {
        std::string name;
        int med, clim, urb, gov;
        double popM;
    };

    std::vector<RawCountry> raw = {
        {"AUSTRALIA",          5, 4, 5, 4, 26.0},
        {"BELARUS",            3, 2, 3, 3, 9.0},
        {"BRAZIL",             2, 5, 4, 2, 213.0},
        {"CANADA",             5, 1, 4, 4, 39.0},
        {"CHINA",              3, 3, 5, 5, 1400.0},
        {"EAST",               2, 4, 3, 2, 120.0},
        {"GREENLAND",          3, 1, 1, 3, 0.06},
        {"ICELAND",            4, 1, 2, 4, 0.4},
        {"INDIA",              2, 5, 4, 2, 1400.0},
        {"JAPAN",              5, 3, 5, 5, 125.0},
        {"KAZAKHSTAN",         3, 2, 3, 3, 19.0},
        {"M AFRICA",           2, 5, 2, 2, 180.0},
        {"MADAGASCAR",         2, 5, 2, 2, 30.0},
        {"MEXICO",             3, 4, 4, 3, 128.0},
        {"MIDDLE EAST",        3, 5, 4, 3, 260.0},
        {"MONGOLIA",           2, 2, 2, 3, 3.5},
        {"N AFRICA",           2, 5, 3, 2, 250.0},
        {"N SOUTH AMERICA",    2, 5, 3, 2, 60.0},
        {"NEW ZELAND",         5, 3, 3, 5, 5.0},
        {"OCEANIA",            2, 5, 2, 2, 12.0},
        {"RUSSIA",             3, 2, 3, 3, 144.0},
        {"S AFRICA",           3, 4, 3, 3, 60.0},
        {"SCANDINAVIA",        5, 1, 4, 5, 27.0},
        {"SW SOUTH AMERICA",   3, 3, 3, 3, 50.0},
        {"TURKEY",             3, 4, 4, 3, 85.0},
        {"UK",                 5, 2, 5, 4, 67.0},
        {"UKRAINE",            3, 2, 3, 3, 37.0},
        {"USA",                5, 3, 5, 3, 331.0},
        {"W EUROPE",           5, 3, 5, 4, 450.0},
    };

    for (const auto& r : raw) {
        Country c;
        c.name = r.name;
        c.params.medicine = r.med;
        c.params.climate = r.clim;
        c.params.urbanization = r.urb;
        c.params.governmentReaction = r.gov;
        c.pop = Population(r.popM * 1'000'000.0);
        world.countries.push_back(c);
    }

    world.connections = buildInitialConnections(world.countries);

    // Параметры вируса
    world.virus.infectivity = 0.3;
    world.virus.lethality = 0.0;
    world.virus.vaccineDifficulty = 0.5;
    world.virus.incubationPeriod = 5.0;    
    world.virus.infectiousPeriod = 14.0;   
    // Модификаторы климата: холодный, умеренный, теплый, жаркий, тропический
    world.virus.climateModifiers = {0.7, 0.9, 1.0, 1.1, 1.2};

    // Параметры человечества
    world.humanity.scientistCommitment = 0.5;
    world.humanity.awareness = 0.0;
    world.humanity.developmentDifficultyMod = 0.0;

    // Параметры вакцины
    world.vaccine.progress = 0.0;
    world.vaccine.spreadRate = 0.01;
    world.vaccine.isReady = false;
    world.vaccine.efficacy = 0.95;

    return world;
}

void updateVaccineProgress(World& world) {
    if (world.vaccine.isReady) return;

    double baseSpeed = 0.1;
    double scientistFactor = world.humanity.scientistCommitment;

    double avgMedicine = 0.0;
    for (const auto& country : world.countries) {
        avgMedicine += country.params.medicine / 5.0;
    }
    avgMedicine /= world.countries.size();

    double difficultyFactor = 1.0 - world.virus.vaccineDifficulty * 0.5;

    double dailyProgress = baseSpeed * scientistFactor * avgMedicine * difficultyFactor;

    dailyProgress *= (1.0 + world.humanity.awareness * 0.5);

    world.vaccine.updateProgress(dailyProgress);
}

void updateAwareness(World& world) {
    double totalInfected = 0.0;
    double totalDead = 0.0;
    double totalPopulation = 0.0;

    for (const auto& country : world.countries) {
        totalInfected += country.pop.infected + country.pop.exposed;
        totalDead += country.pop.dead;
        totalPopulation += country.pop.initial;
    }

    double infectionAwareness = totalInfected / totalPopulation * 10.0;
    double deathAwareness = totalDead / totalPopulation * 20.0;

    double targetAwareness = std::min(1.0, infectionAwareness + deathAwareness);
    double awarenessGrowth = (targetAwareness - world.humanity.awareness) * 0.1;

    world.humanity.awareness += awarenessGrowth;
}

void propagateBetweenCountries(World& world) {
    const double transmissionProbability = 0.001;

    for (auto& conn : world.connections) {
        if (!conn.isActive) continue; 

        Country& fromCountry = world.countries[conn.from];
        Country& toCountry = world.countries[conn.to];

        if (fromCountry.bordersClosed || toCountry.bordersClosed) {
            conn.isActive = false;
            continue;
        }

        double transportFactor = (fromCountry.params.getTransportFactor() +
                                  toCountry.params.getTransportFactor()) / 2.0;
        double effectiveVolume = conn.transportVolume * transportFactor;

        double infectedFraction = fromCountry.pop.infected / fromCountry.pop.initial;

        double infectedTravelers = effectiveVolume * infectedFraction * transmissionProbability;

        if (infectedTravelers > 0.1) {
            toCountry.pop.exposed += infectedTravelers;
        }
    }
}

void updateEpidemicModel(World& world) {
    const auto& virus = world.virus;
    const auto& vaccine = world.vaccine;

    double sigma = 1.0 / virus.incubationPeriod;  
    double gamma = 1.0 / virus.infectiousPeriod;  

    for (auto& country : world.countries) {
        Population& pop = country.pop;
        double N = pop.initial;

        if (N <= 0) continue;

        double S = pop.susceptible;
        double E = pop.exposed;
        double I = pop.infected;
        double R = pop.recovered;
        double D = pop.dead;

        double climateMult = virus.getClimateModifier(country.params.climate);
        double urbanizationFactor = country.params.getTransportFactor();

        double effectiveBeta = virus.infectivity * climateMult * urbanizationFactor;

        double newExposed = effectiveBeta * (S / N) * I;
        if (newExposed > S) newExposed = S;
        if (newExposed < 0) newExposed = 0;

        double becomingInfectious = sigma * E;
        if (becomingInfectious > E) becomingInfectious = E;

        double naturalRecoveryRate = 0.0;
        double vaccineRecoveryBonus = vaccine.isReady ? 0.1 : 0.0;
        double recoveryRate = naturalRecoveryRate + vaccineRecoveryBonus;

        double deathRate = gamma * virus.lethality;
        double newDeaths = deathRate * I;
        double newRecovered = recoveryRate * I;

        double totalRemoving = newDeaths + newRecovered;
        if (totalRemoving > I) {
            double scale = I / totalRemoving;
            newDeaths *= scale;
            newRecovered *= scale;
        }

        double newVaccinated = 0.0;
        if (vaccine.isReady) {
            double vaccRate = country.params.getVaccineSpeed() * vaccine.spreadRate;
            vaccRate *= vaccine.efficacy; 
            newVaccinated = vaccRate * S;
            if (newVaccinated > S) newVaccinated = S;
        }

        S = S - newExposed - newVaccinated;
        E = E + newExposed - becomingInfectious;
        I = I + becomingInfectious - newRecovered - newDeaths;
        R = R + newRecovered + newVaccinated;
        D = D + newDeaths;

        if (S < 0) S = 0;
        if (E < 0) E = 0;
        if (I < 0) I = 0;
        if (R < 0) R = 0;
        if (D < 0) D = 0;

        pop.susceptible = S;
        pop.exposed = E;
        pop.infected = I;
        pop.recovered = R;
        pop.dead = D;
    }
}

/**
 * Полный шаг симуляции (один день)
 */
void simulateDay(World& world) {

    updateAwareness(world);
    world.checkBorderClosures();
    propagateBetweenCountries(world);
    updateEpidemicModel(world);
    updateVaccineProgress(world);
}
}