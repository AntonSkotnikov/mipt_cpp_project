#pragma once

/**
 * @file EventGenerator.hpp
 * @brief Генератор событий для игры Plague Inc. (двухигровая версия)
 *
 * Генерирует следующие типы событий:
 * - Первое заражение (появление зараженных в выбранной стране)
 * - Отправка транспорта между странами (с шансом заражения)
 * - Закрытие границ (зависит от реакции властей и осведомленности)
 * - Начисление игровой валюты (DNA) игрокам
 */

#include "MatModel.hpp"
#include <random>
#include <cstdint>

namespace plague {

// ========== ТИПЫ СОБЫТИЙ ==========
enum class EventType {
    FirstInfection,     // Первое заражение в стране
    TransportMovement,  // Перемещение транспорта между странами
    BorderClosure,      // Закрытие границ страной
    DNAGrant,           // Начисление DNA игроку
    None                // Событие не произошло
};

// ========== РЕЗУЛЬТАТ СОБЫТИЯ ==========
struct EventResult {
    EventType type;
    std::string description;

    // Для первого заражения
    size_t countryIndex = 0;
    int infectedCount = 0;

    // Для транспорта
    size_t fromCountry = 0;
    size_t toCountry = 0;
    bool transportedInfected = false;
    int infectedTransported = 0;

    // Для закрытия границ
    size_t closedCountry = 0;

    // Для начисления DNA
    int playerIndex = 0;  // 0 = Pathogen, 1 = Humanity
    int dnaAmount = 0;

    EventResult() : type(EventType::None) {}
};

// ========== ПАРАМЕТРЫ ГЕНЕРАТОРА ==========
struct EventGeneratorParams {
    // Параметры для первого заражения
    int baseFirstInfectionCount = 100;  // Базовое количество зараженных при первом заражении

    // Параметры для транспорта
    double baseInfectedTransportChance = 0.3;  // Базовый шанс перевозки зараженных
    double infectivityModifier = 0.1;          // Модификатор от заразности вируса

    // Параметры для закрытия границ
    double baseBorderCloseChance = 0.05;       // Базовый шанс закрытия границ в день
    double governmentReactionModifier = 0.02;  // Модификатор от реакции властей
    double awarenessModifier = 0.03;           // Модификатор от осведомленности

    // Параметры для начисления DNA
    double dnaGrantChance = 0.3;               // Шанс начисления DNA в день
    int dnaGrantInterval = 3;                  // Минимальный интервал между начислениями (дни)
    int baseDNAAmount = 5;                     // Базовое количество DNA
    int pathogenDNAFromInfections = 1;         // DNA за определенное количество заражений
    int humanityDNAFromVaccine = 2;            // DNA за прогресс вакцины
};

// ========== ГЕНЕРАТОР СОБЫТИЙ ==========
class EventGenerator {
public:
    EventGenerator();

    /**
     * @brief Установить начальное значение для генератора случайных чисел
     */
    void seed(uint32_t seed);

    /**
     * @brief Сгенерировать событие для одного дня симуляции
     * @param world Ссылка на мир
     * @param day Текущий день симуляции
     * @param pathogenDNA Текущее количество DNA патогена
     * @param humanityDNA Текущее количество DNA человечества
     * @return Результат события
     */
    EventResult generateEvent(World& world, int day,
                              int pathogenDNA, int humanityDNA);

    /**
     * @brief Получить параметры генератора
     */
    const EventGeneratorParams& getParams() const { return params_; }

    /**
     * @brief Установить параметры генератора
     */
    void setParams(const EventGeneratorParams& params) { params_ = params; }

private:
    std::mt19937 rng_;
    EventGeneratorParams params_;
    int lastDNAGrantDay_;  // День последнего начисления DNA

    /**
     * @brief Попытка сгенерировать первое заражение
     */
    EventResult tryFirstInfection(World& world);

    /**
     * @brief Попытка сгенерировать перемещение транспорта
     */
    EventResult tryTransportMovement(World& world);

    /**
     * @brief Попытка сгенерировать закрытие границ
     */
    EventResult tryBorderClosure(World& world);

    /**
     * @brief Попытка сгенерировать начисление DNA
     */
    EventResult tryDNAGrant(World& world, int day, int pathogenDNA, int humanityDNA);

    /**
     * @brief Получить случайное число от 0 до 1
     */
    double randomDouble();

    /**
     * @brief Получить случайное целое число в диапазоне [min, max]
     */
    int randomInt(int min, int max);

    /**
     * @brief Выбрать случайную страну из тех, где еще нет зараженных
     */
    int selectRandomUninfectedCountry(const World& world);

    /**
     * @brief Выбрать случайную страну
     */
    int selectRandomCountry(const World& world);
};

} // namespace plague