#pragma once

/**
 * @file SimulationTypes.hpp
 * @brief Структуры данных для математической модели распространения вируса
 * 
 * Этот файл содержит все основные структуры для симуляции:
 * - Параметры страны
 * - Население (SEIR модель)
 * - Страна
 * - Вирус
 * - Человечество
 * - Вакцина
 * - Связи между странами
 * - Мир
 */

#include <vector>
#include <string>

namespace plague {

// ========== ПАРАМЕТРЫ СТРАНЫ ==========
struct CountryParams {
    int medicine;           // 1-5: уровень развития медицины
    int climate;            // 1-5: климат (1=холодный, 5=тропический)
    int urbanization;       // 1-5: степень урбанизации
    int governmentReaction; // 1-5: скорость реакции властей
    
    // Влияние параметров на игру:
    // - medicine: влияет на количество ученых и скорость распространения вакцины
    // - climate: влияет на скорость распространения вируса
    // - urbanization: влияет на количество транспорта въезжающего/выезжающего
    // - governmentReaction: влияет на скорость закрытия границ
    
    double getScienceFactor() const;      // Коэффициент количества ученых
    double getVaccineSpeed() const;       // Скорость распространения вакцины
    double getTransportFactor() const;    // Объем транспорта
    double getBorderCloseSpeed() const;   // Скорость закрытия границ
};

// ========== НАСЕЛЕНИЕ (SEIR модель) ==========
struct Population {
    double initial;         // Стартовое количество населения
    double susceptible;     // S - Восприимчивые (еще не заражены)
    double exposed;         // E - Зараженные, но не заразные (инкубационный период)
    double infected;        // I - Зараженные и заразные
    double recovered;       // R - Вылеченные или вакцинированные
    double dead;            // D - Умершие
    
    double alive() const;   // Общее количество живых
    
    Population(double total = 0.0);
};

// ========== СТРАНА ==========
struct Country {
    std::string name;
    CountryParams params;
    Population pop;
    double borderOpenness;  // 0.0 = полностью закрыта, 1.0 = полностью открыта
    bool bordersClosed;     // Флаг полного закрытия границ
    
    Country();
};

// ========== ВИРУС ==========
struct Virus {
    double infectivity;         // Заразность (базовый коэффициент передачи β)
    double lethality;           // Летальность (базовый процент смертности)
    double vaccineDifficulty;   // Сложность изготовления вакцины (0-1)
    double incubationPeriod;    // Период инкубации в днях (E -> I)
    double infectiousPeriod;    // Период заразности в днях (I -> R/D)
    std::vector<double> climateModifiers; // Модификаторы для разных климатов
    
    double getClimateModifier(int climateLevel) const;
};

// ========== ЧЕЛОВЕЧЕСТВО ==========
struct Humanity {
    double scientistCommitment;     // Вовлеченность ученых (0-1)
                                    // Влияет на скорость разработки вакцины
    double awareness;               // Осведомленность (0-1)
                                    // Влияет на скорость закрытия границ
    double developmentDifficultyMod;// Модификатор сложности разработки вакцины
    
    double getGlobalScientists() const;         // Количество ученых
    double getBorderCloseThreshold() const;     // Порог для закрытия границ
};

// ========== ВАКЦИНА ==========
struct Vaccine {
    double progress;          // Степень готовности (0-100%)
    double spreadRate;        // Скорость распространения вакцинации
    bool isReady;             // Готова ли к применению
    double efficacy;          // Эффективность (0-1)
    
    Vaccine();
    void updateProgress(double delta);  // Обновить прогресс
};

// ========== СВЯЗЬ МЕЖДУ СТРАНАМИ ==========
struct CountryConnection {
    size_t from;              // Индекс страны-источника
    size_t to;                // Индекс страны-назначения
    double transportVolume;   // Объем транспорта (людей в день)
    bool isActive;            // Активна ли связь (границы открыты)
    
    // Связь обрывается при закрытии границ одной из стран
    CountryConnection(size_t f = 0, size_t t = 0, double volume = 1000.0);
};

// ========== МИР ==========
class World {
public:
    std::vector<Country> countries;         // Список стран
    std::vector<CountryConnection> connections; // Транспортные связи
    Virus virus;                            // Параметры вируса
    Humanity humanity;                      // Параметры человечества
    Vaccine vaccine;                        // Параметры вакцины
    
    // Управление границами
    int getCountryIndex(const std::string& name) const;
    void closeBorder(size_t i, size_t j);       // Закрыть границу между двумя странами
    void closeAllBorders(size_t countryIdx);    // Закрыть все границы страны
    void checkBorderClosures();                 // Автоматическая проверка закрытия границ
};

} // namespace plague
