#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace plague {

// ========== ТИПЫ СОБЫТИЙ ДЛЯ КЛИЕНТА ==========
enum class EventType {
    NEWS_FIRST_INFECTION,      // Первое заражение в стране
    NEWS_COUNTRY_INFECTED,     // Вся страна заражена (>95%)
    NEWS_FIRST_DEATH,          // Первая смерть от вируса
    NEWS_COUNTRY_EXTINCT,      // Страна вымерла
    NEWS_DISASTER,             // Природное бедствие
    NEWS_EVENT_GLOBAL,         // Глобальное событие (Олимпиада и т.д.)
    ACTION_DNA_CLICK,          // Требуется клик для получения DNA
    ACTION_BORDER_UPGRADE      // Предложение улучшить границы
};

// ========== НОВОСТЬ/СОБЫТИЕ ДЛЯ ОТПРАВКИ КЛИЕНТУ ==========
struct GameNews {
    EventType type;
    std::string title;
    std::string message;
    std::string countryName;
    uint64_t countryId;
    uint64_t eventId;
    int day;

    GameNews(EventType t, const std::string& ttl, const std::string& msg,
             const std::string& cName = "", uint64_t cId = 0, uint64_t eId = 0, int d = 0)
        : type(t), title(ttl), message(msg), countryName(cName),
          countryId(cId), eventId(eId), day(d) {}
};

// ========== ПАРАМЕТРЫ СТРАНЫ ==========
struct CountryParams {
    int medicine;           // 1-5: уровень развития медицины
    int climate;            // 1-5: климат (1=холодный, 5=тропический)
    int urbanization;       // 1-5: степень урбанизации
    int governmentReaction; // 1-5: скорость реакции властей

    double getScienceFactor() const;
    double getVaccineSpeed() const;
    double getTransportFactor() const;
    double getBorderCloseSpeed() const;
};

// ========== НАСЕЛЕНИЕ (SEIR модель) ==========
struct Population {
    double initial;
    double susceptible;
    double exposed;
    double infected;
    double recovered;
    double dead;

    double alive() const;
    Population(double total = 0.0);
};

// ========== СТРАНА ==========
struct Country {
    std::string name;
    CountryParams params;
    Population pop;
    double borderOpenness;
    bool bordersClosed;

    Country();
};

// ========== ВИРУС ==========
struct Virus {
    double infectivity;
    double lethality;
    double vaccineDifficulty;
    double incubationPeriod;
    double infectiousPeriod;
    std::vector<double> climateModifiers;

    double getClimateModifier(int climateLevel) const;
};

// ========== ЧЕЛОВЕЧЕСТВО ==========
struct Humanity {
    double scientistCommitment;
    double awareness;
    double developmentDifficultyMod;

    double getGlobalScientists() const;
    double getBorderCloseThreshold() const;
};

// ========== ВАКЦИНА ==========
struct Vaccine {
    double progress;
    double spreadRate;
    bool isReady;
    double efficacy;

    Vaccine();
    void updateProgress(double delta);
};

// ========== СВЯЗЬ МЕЖДУ СТРАНАМИ ==========
struct CountryConnection {
    size_t from;
    size_t to;
    double transportVolume;
    bool isActive;

    CountryConnection(size_t f = 0, size_t t = 0, double volume = 1000.0);
};

// ========== МИР ==========
class World {
public:
    std::vector<Country> countries;
    std::vector<CountryConnection> connections;
    Virus virus;
    Humanity humanity;
    Vaccine vaccine;

    std::vector<GameNews> newsQueue;
    uint64_t nextEventId;

    World() : nextEventId(1) {}

    void addNews(const GameNews& news) {
        newsQueue.push_back(news);
    }

    int getCountryIndex(const std::string& name) const;
    void closeBorder(size_t i, size_t j);
    void closeAllBorders(size_t countryIdx);
    void checkBorderClosures();
};

std::vector<CountryConnection> buildInitialConnections(const std::vector<Country>& countries);
World initializeWorld();
void simulateDay(World& world);  // Функция симуляции одного дня

} // namespace plague
