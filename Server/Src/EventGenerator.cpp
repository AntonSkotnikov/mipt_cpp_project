#include "EventGenerator.hpp"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace plague {

// ========== КОНСТРУКТОР ==========
EventGenerator::EventGenerator()
    : rng_(std::random_device{}()), lastDNAGrantDay_(-10) {
}

void EventGenerator::seed(uint32_t seed) {
    rng_.seed(seed);
}

double EventGenerator::randomDouble() {
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(rng_);
}

int EventGenerator::randomInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(rng_);
}

int EventGenerator::selectRandomUninfectedCountry(const World& world) {
    std::vector<int> uninfectedIndices;

    for (size_t i = 0; i < world.countries.size(); ++i) {
        const auto& country = world.countries[i];
        if (country.pop.infected < 1.0 && country.pop.exposed < 1.0) {
            uninfectedIndices.push_back(static_cast<int>(i));
        }
    }

    if (uninfectedIndices.empty()) return -1;

    size_t randomIdx = static_cast<size_t>(randomInt(0, static_cast<int>(uninfectedIndices.size()) - 1));
    return uninfectedIndices[randomIdx];
}

int EventGenerator::selectRandomInfectedCountry(const World& world) {
    std::vector<int> infectedIndices;

    for (size_t i = 0; i < world.countries.size(); ++i) {
        const auto& country = world.countries[i];
        if (country.pop.infected >= 1.0 || country.pop.exposed >= 1.0) {
            infectedIndices.push_back(static_cast<int>(i));
        }
    }

    if (infectedIndices.empty()) return -1;

    size_t randomIdx = static_cast<size_t>(randomInt(0, static_cast<int>(infectedIndices.size()) - 1));
    return infectedIndices[randomIdx];
}

// ========== ГЛАВНАЯ ФУНКЦИЯ ГЕНЕРАЦИИ ==========
EventResult EventGenerator::generateEvent(World& world, int day,
                                          int pathogenDNA, int humanityDNA) {
    (void)pathogenDNA;

    EventResult result;

    updateActiveEvents(world);

    bool hasInfection = false;
    for (const auto& country : world.countries) {
        if (country.pop.infected > 0 || country.pop.exposed > 0) {
            hasInfection = true;
            break;
        }
    }

    if (!hasInfection) {
        result = tryFirstInfection(world);
        if (result.eventId != 0) {
            return result;
        }
    }

    result = trySpecialEvent(world, day);
    if (result.eventId != 0) {
        return result;
    }

    result = tryTransportMovement(world);
    if (result.eventId != 0) {
        return result;
    }

    result = tryDNAClickOpportunity(world, day);
    if (result.eventId != 0) {
        return result;
    }

    result = tryBorderUpgrade(world, humanityDNA);
    if (result.eventId != 0) {
        return result;
    }

    return EventResult();
}

// ========== ПЕРВОЕ ЗАРАЖЕНИЕ ==========
EventResult EventGenerator::tryFirstInfection(World& world) {
    EventResult result;

    int countryIdx = selectRandomUninfectedCountry(world);
    if (countryIdx < 0) {
        return result;
    }

    double infectionChance = 0.15;

    if (randomDouble() > infectionChance) {
        return result;
    }

    int infectedCount = params_.baseFirstInfectionCount + randomInt(-20, 50);
    infectedCount = std::max(10, infectedCount);

    Country& country = world.countries[static_cast<size_t>(countryIdx)];
    if (country.pop.susceptible >= infectedCount) {
        country.pop.susceptible -= infectedCount;
        country.pop.exposed += infectedCount;

        result.eventId = world.nextEventId++;
        result.type = EventType::NEWS_FIRST_INFECTION;
        result.countryIndex = static_cast<size_t>(countryIdx);
        result.infectedCount = infectedCount;

        std::ostringstream ss;
        ss << "First infection in " << country.name
           << "! " << infectedCount << " people infected.";
        result.description = ss.str();

        GameNews news(
            EventType::NEWS_FIRST_INFECTION,
            "First Infection!",
            "The virus has been detected in " + country.name + ". " +
            std::to_string(infectedCount) + " people are now exposed.",
            country.name,
            static_cast<uint64_t>(countryIdx),
            result.eventId,
            0  
        );
        world.addNews(news);
    }

    return result;
}

// ========== ТРАНСПОРТ ==========
EventResult EventGenerator::tryTransportMovement(World& world) {
    EventResult result;

    if (world.connections.empty()) {
        return result;
    }

    std::vector<size_t> activeConnections;
    for (size_t i = 0; i < world.connections.size(); ++i) {
        if (world.connections[i].isActive) {
            activeConnections.push_back(i);
        }
    }

    if (activeConnections.empty()) {
        return result;
    }

    size_t connIdx = activeConnections[static_cast<size_t>(
        randomInt(0, static_cast<int>(activeConnections.size()) - 1))];

    auto& conn = world.connections[connIdx];
    auto& fromCountry = world.countries[conn.from];
    auto& toCountry = world.countries[conn.to];

    double infectedFraction = fromCountry.pop.infected /
                              std::max(1.0, fromCountry.pop.initial);

    if (infectedFraction < 0.0001) {
        return result;
    }

    int travelersPerDay = static_cast<int>(conn.transportVolume *
                                           fromCountry.params.getTransportFactor());

    bool transmission = checkInfectionTransmission(
        infectedFraction,
        travelersPerDay,
        world.virus.infectivity,
        1.0
    );

    result.eventId = world.nextEventId++;
    result.type = EventType::NEWS_FIRST_INFECTION; 
    result.fromCountry = conn.from;
    result.toCountry = conn.to;
    result.routeType = RouteType::Air;
    result.totalTravelers = travelersPerDay;

    std::ostringstream ss;
    ss << "Transport from " << fromCountry.name
       << " to " << toCountry.name << " (" << travelersPerDay << " passengers)";

    if (transmission) {
        int infectedCount = std::max(1, static_cast<int>(
            travelersPerDay * infectedFraction * world.virus.infectivity
        ));

        toCountry.pop.exposed += infectedCount;

        result.transportedInfected = true;
        result.infectedTransported = infectedCount;

        ss << " - transported " << infectedCount << " infected!";

        if (toCountry.pop.infected < 1.0 && toCountry.pop.exposed <= static_cast<double>(infectedCount + 10)) {
            GameNews news(
                EventType::NEWS_FIRST_INFECTION,
                "Virus Spreads!",
                "The virus has arrived in " + toCountry.name + " via transport from " + fromCountry.name + ". " +
                std::to_string(infectedCount) + " new cases detected.",
                toCountry.name,
                static_cast<uint64_t>(conn.to),
                result.eventId,
                0
            );
            world.addNews(news);
        }
    } else {
        ss << " - no infected.";
    }

    result.description = ss.str();

    return result;
}

bool EventGenerator::checkInfectionTransmission(double infectedFraction,
                                                  int travelers,
                                                  double virusInfectivity,
                                                  double routeSpeedMod) {
    double expectedInfected = travelers * infectedFraction;

    double baseChance = params_.baseInfectedTransportChance;
    double infectivityMod = virusInfectivity * params_.infectivityModifier;
    double totalChance = baseChance + infectivityMod * routeSpeedMod;

    if (expectedInfected > 1.0) {
        totalChance *= std::min(2.0, 1.0 + std::log(expectedInfected) * 0.3);
    }

    totalChance = std::min(0.95, totalChance);

    return randomDouble() < totalChance;
}

// ========== СПЕЦИАЛЬНЫЕ СОБЫТИЯ ==========
EventResult EventGenerator::trySpecialEvent(World& world, int day) {
    EventResult result;

    if (randomDouble() > params_.specialEventChance) {
        return result;
    }

    int eventType = randomInt(0, 5);
    SpecialEventType specType = static_cast<SpecialEventType>(eventType);

    SpecialEventEffect effect;

    switch(specType) {
        case SpecialEventType::Olympics:
            effect.type = SpecialEventType::Olympics;
            effect.duration = randomInt(7, 14);
            effect.infectivityMod = 1.0;
            effect.transportMod = 2.5;
            effect.vaccineMod = 1.0;
            effect.bordersForcedOpen = true;
            effect.description = "Olympic Games! Increased traffic between countries.";
            break;

        case SpecialEventType::HeatWave:
            effect.type = SpecialEventType::HeatWave;
            effect.duration = randomInt(5, 10);
            effect.infectivityMod = 1.3;
            effect.transportMod = 0.8;
            effect.vaccineMod = 0.9;
            effect.bordersForcedOpen = false;
            effect.description = "Heat wave! Virus spreads faster.";
            break;

        case SpecialEventType::ColdSnap:
            effect.type = SpecialEventType::ColdSnap;
            effect.duration = randomInt(5, 10);
            effect.infectivityMod = 0.7;
            effect.transportMod = 0.7;
            effect.vaccineMod = 1.0;
            effect.bordersForcedOpen = false;
            effect.description = "Cold snap! Human activity reduced.";
            break;

        case SpecialEventType::NaturalDisaster:
            effect.type = SpecialEventType::NaturalDisaster;
            effect.duration = randomInt(3, 7);
            effect.infectivityMod = 1.5;
            effect.transportMod = 1.2;
            effect.vaccineMod = 0.8;
            effect.bordersForcedOpen = false;
            effect.description = "Natural disaster! Chaos and migration.";
            break;

        case SpecialEventType::MedicalConference:
            effect.type = SpecialEventType::MedicalConference;
            effect.duration = randomInt(3, 5);
            effect.infectivityMod = 1.0;
            effect.transportMod = 1.0;
            effect.vaccineMod = 1.5;
            effect.bordersForcedOpen = false;
            effect.description = "Medical conference! Scientists sharing knowledge.";
            break;

        case SpecialEventType::PoliticalSummit:
            effect.type = SpecialEventType::PoliticalSummit;
            effect.duration = randomInt(2, 4);
            effect.infectivityMod = 1.0;
            effect.transportMod = 1.3;
            effect.vaccineMod = 1.0;
            effect.bordersForcedOpen = true;
            effect.description = "Political summit! Borders remain open.";
            break;
    }

    activeEvents_.emplace_back(effect, effect.duration);

    result.eventId = world.nextEventId++;
    result.type = EventType::NEWS_EVENT_GLOBAL;
    result.specialEvent = effect;
    result.description = effect.description;

    std::string title = "Global Event!";
    GameNews news(
        EventType::NEWS_EVENT_GLOBAL,
        title,
        effect.description,
        "",
        0,
        result.eventId,
        day
    );
    world.addNews(news);

    return result;
}

// ========== ОБНОВЛЕНИЕ АКТИВНЫХ СОБЫТИЙ ==========
void EventGenerator::updateActiveEvents(World& world) {
    for (auto it = activeEvents_.begin(); it != activeEvents_.end();) {
        it->daysRemaining--;

        if (it->effect.bordersForcedOpen) {
            for (auto& conn : world.connections) {
                conn.isActive = true;
            }
            for (auto& country : world.countries) {
                country.bordersClosed = false;
                country.borderOpenness = 1.0;
            }
        }

        if (it->daysRemaining <= 0) {
            it = activeEvents_.erase(it);
        } else {
            ++it;
        }
    }
}

// ========== ВОЗМОЖНОСТЬ ПОЛУЧИТЬ DNA ==========
EventResult EventGenerator::tryDNAClickOpportunity(World& world, int day) {
    EventResult result;

    if (day - lastDNAGrantDay_ < params_.minDaysBetweenDNA) {
        return result;
    }

    if (randomDouble() > params_.dnaClickChance) {
        return result;
    }

    int countryIdx = selectRandomInfectedCountry(world);
    if (countryIdx < 0) {
        return result;
    }

    const auto& country = world.countries[static_cast<size_t>(countryIdx)];

    double infectionRate = (country.pop.infected + country.pop.exposed) /
                           std::max(1.0, country.pop.initial);
    
    int randomBase = randomInt(3, 8);
    int timeBonus = day / 5;
    int infectionBonus = static_cast<int>(infectionRate * 15);
    int dnaAmount = randomBase + timeBonus + infectionBonus;
    dnaAmount = std::max(1, dnaAmount);

    result.eventId = world.nextEventId++;
    result.type = EventType::ACTION_DNA_CLICK;
    result.highlightedCountry = static_cast<size_t>(countryIdx);
    result.dnaAmount = dnaAmount;
    result.playerIndex = -1;

    std::ostringstream ss;
    ss << "Outbreak in " << country.name << "! "
       << "First player to click gets " << dnaAmount << " DNA!";
    result.description = ss.str();

    GameNews news(
        EventType::ACTION_DNA_CLICK,
        "DNA Opportunity!",
        "Click on " + country.name + " to get " + std::to_string(dnaAmount) + " DNA! First player wins.",
        country.name,
        static_cast<uint64_t>(countryIdx),
        result.eventId,
        day
    );
    world.addNews(news);

    lastDNAGrantDay_ = day;

    return result;
}

// ========== ОБРАБОТКА КЛИКА ==========
int EventGenerator::handleCountryClick(size_t countryIdx, int playerIndex) {
    (void)countryIdx;
    (void)playerIndex;

    return params_.baseDNAAmount;
}

// ========== АПГРЕЙД ЗАКРЫТИЯ ГРАНИЦ ==========
EventResult EventGenerator::tryBorderUpgrade(World& world, int humanityDNA) {
    EventResult result;

    if (humanityDNA < params_.borderUpgradeCost) {
        return result;
    }

    if (randomDouble() > params_.borderUpgradeChance) {
        return result;
    }

    result.eventId = world.nextEventId++;
    result.type = EventType::ACTION_BORDER_UPGRADE;
    result.dnaAmount = -params_.borderUpgradeCost;
    result.playerIndex = 1;

    std::ostringstream ss;
    ss << "Humanity purchased border closure upgrade for "
       << params_.borderUpgradeCost << " DNA!";
    result.description = ss.str();

    GameNews news(
        EventType::ACTION_BORDER_UPGRADE,
        "Border Upgrade!",
        "Humanity has purchased a border closure upgrade for " +
        std::to_string(params_.borderUpgradeCost) + " DNA.",
        "",
        0,
        result.eventId,
        0
    );
    world.addNews(news);

    return result;
}

// ========== КОНСТРУКТОР SPECIAL EVENT EFFECT ==========
SpecialEventEffect::SpecialEventEffect()
    : type(SpecialEventType::Olympics), duration(0),
      infectivityMod(1.0), transportMod(1.0), vaccineMod(1.0),
      bordersForcedOpen(false) {}

} 
