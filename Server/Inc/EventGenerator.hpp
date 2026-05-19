#pragma once

#include "SimulationTypes.hpp"
#include <random>
#include <cstdint>
#include <vector>
#include <string>

namespace plague {

// ========== ТИПЫ МАРШРУТОВ ==========
enum class RouteType {
    Air,  
    Sea,
    Land        
};

// ========== ПАРАМЕТРЫ МАРШРУТА ==========
struct RouteParams {
    RouteType type;
    int routesPerDay;       // Количество рейсов/путей в день
    int passengersPerRoute; // Среднее количество пассажиров за рейс
    double speedModifier;   // Модификатор скорости распространения

    RouteParams() : type(RouteType::Air), routesPerDay(1), passengersPerRoute(100), speedModifier(1.0) {}
    RouteParams(RouteType t, int rpd, int ppr, double sm)
        : type(t), routesPerDay(rpd), passengersPerRoute(ppr), speedModifier(sm) {}
};

// ========== ТИПЫ СПЕЦИАЛЬНЫХ СОБЫТИЙ ==========
enum class SpecialEventType {
    Olympics,               // Олимпийские игры (увеличивает транспорт)
    HeatWave,               // Жара (влияет на распространение)
    ColdSnap,               // Заморозки (влияет на распространение)
    NaturalDisaster,        // Природное бедствие (хаос, миграция)
    MedicalConference,      // Медицинская конференция (ускоряет вакцину)
    PoliticalSummit         // Политический саммит (границы открыты)
};

// ========== ЭФФЕКТ СПЕЦИАЛЬНОГО СОБЫТИЯ ==========
struct SpecialEventEffect {
    SpecialEventType type;
    int duration;           // Длительность в днях
    double infectivityMod;  // Модификатор заразности
    double transportMod;    // Модификатор транспорта
    double vaccineMod;      // Модификатор разработки вакцины
    bool bordersForcedOpen; // Принудительно открытые границы
    std::string description;

    SpecialEventEffect();
};


struct EventResult {
    EventType type;  
    std::string description;

    // Для первого заражения
    size_t countryIndex = 0;
    int infectedCount = 0;

    // Для транспорта
    size_t fromCountry = 0;
    size_t toCountry = 0;
    RouteType routeType = RouteType::Air;
    bool transportedInfected = false;
    int infectedTransported = 0;
    int totalTravelers = 0;

    // Для специальных событий
    SpecialEventEffect specialEvent;

    // Для DNA
    size_t highlightedCountry = 0;  
    int dnaAmount = 0;
    int playerIndex = -1;           // -1 = еще не выбран (ждём клика)

    // Для закрытия границ
    size_t closedCountry = 0;

    // ID события для реакции клиента
    uint64_t eventId = 0;

    EventResult() : type(EventType::NEWS_FIRST_INFECTION) {}
};


struct EventGeneratorParams {
    // Параметры для первого заражения
    int baseFirstInfectionCount = 100;

    // Параметры для транспорта
    double baseInfectedTransportChance = 0.3;
    double infectivityModifier = 0.1;

    // Параметры для специальных событий
    double specialEventChance = 0.02;  

    // Параметры для DNA
    double dnaClickChance = 0.15;      
    int baseDNAAmount = 5;
    int minDaysBetweenDNA = 2;

    // Параметры для апгрейда закрытия границ
    double borderUpgradeChance = 0.05;
    int borderUpgradeCost = 10;
};

struct ActiveSpecialEvent {
    SpecialEventEffect effect;
    int daysRemaining;

    ActiveSpecialEvent() : daysRemaining(0) {}
    ActiveSpecialEvent(const SpecialEventEffect& e, int duration)
        : effect(e), daysRemaining(duration) {}
};

class EventGenerator {
public:
    EventGenerator();

    void seed(uint32_t seed);

    EventResult generateEvent(World& world, int day,
                              int pathogenDNA, int humanityDNA);

    int handleCountryClick(size_t countryIdx, int playerIndex);

    void updateActiveEvents(World& world);

    const EventGeneratorParams& getParams() const { return params_; }
    void setParams(const EventGeneratorParams& params) { params_ = params; }

private:
    std::mt19937 rng_;
    EventGeneratorParams params_;
    int lastDNAGrantDay_;
    std::vector<ActiveSpecialEvent> activeEvents_;

    EventResult tryFirstInfection(World& world);
    EventResult tryTransportMovement(World& world);
    EventResult trySpecialEvent(World& world, int day);
    EventResult tryDNAClickOpportunity(World& world, int day);
    EventResult tryBorderUpgrade(World& world, int humanityDNA);

    double randomDouble();
    int randomInt(int min, int max);
    int selectRandomUninfectedCountry(const World& world);
    int selectRandomInfectedCountry(const World& world);

    bool checkInfectionTransmission(double infectedFraction,
                                    int travelers,
                                    double virusInfectivity,
                                    double routeSpeedMod);
};

} 