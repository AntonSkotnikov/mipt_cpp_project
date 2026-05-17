/**
 * @file EventGeneratorTest.cpp
 * @brief Тест генератора событий для игры Plague Inc.
 * 
 * Запуск: g++ -std=c++20 -DMATMODEL_STANDALONE=0 -DEVENT_GENERATOR_TEST=1 \
 *       -ICommon/Inc -IServer/Inc Server/Src/MatModel.cpp Server/Src/EventGenerator.cpp \
 *       EventGeneratorTest.cpp -o event_test && ./event_test
 */

#include "MatModel.hpp"
#include "EventGenerator.hpp"
#include <iostream>
#include <iomanip>
#include <string>

using namespace plague;

void printEvent(const EventResult& event, int day) {
    std::cout << "День " << day << ": ";
    
    switch (event.type) {
        case EventType::FirstInfection:
            std::cout << "[ЗАРАЖЕНИЕ] " << event.description;
            break;
        case EventType::TransportMovement:
            std::cout << "[ТРАНСПОРТ] " << event.description;
            break;
        case EventType::BorderClosure:
            std::cout << "[ГРАНИЦЫ] " << event.description;
            break;
        case EventType::DNAGrant:
            std::cout << "[DNA] " << event.description;
            break;
        case EventType::None:
            std::cout << "[НЕТ СОБЫТИЙ]";
            break;
    }
    std::cout << std::endl;
}

void printWorldStats(const World& world) {
    double totalInfected = 0, totalDead = 0, totalRecovered = 0;
    int closedBorders = 0;
    
    for (const auto& country : world.countries) {
        totalInfected += country.pop.infected + country.pop.exposed;
        totalDead += country.pop.dead;
        totalRecovered += country.pop.recovered;
        if (country.bordersClosed) closedBorders++;
    }
    
    std::cout << "  Статистика: Заражено=" << static_cast<int>(totalInfected)
              << ", Умерло=" << static_cast<int>(totalDead)
              << ", Выздоровело=" << static_cast<int>(totalRecovered)
              << ", Вакцина=" << std::fixed << std::setprecision(1) << world.vaccine.progress << "%"
              << ", Осведомленность=" << std::setprecision(1) << (world.humanity.awareness * 100) << "%"
              << ", Закрыто границ=" << closedBorders << "/" << world.countries.size()
              << std::endl;
}

int main() {
    std::cout << "=== ТЕСТ ГЕНЕРАТОРА СОБЫТИЙ ===" << std::endl << std::endl;
    
    // Инициализация мира
    World world = initializeWorld();
    
    // Инициализация генератора событий
    EventGenerator generator;
    generator.seed(42); // Фиксированный seed для воспроизводимости
    
    // Параметры ДНК игроков
    int pathogenDNA = 0;
    int humanityDNA = 0;
    
    // Симуляция
    int totalDays = 60;
    
    std::cout << "Начало симуляции на " << totalDays << " дней..." << std::endl;
    std::cout << std::string(80, '-') << std::endl;
    
    for (int day = 1; day <= totalDays; ++day) {
        // Генерируем событие
        EventResult event = generator.generateEvent(world, day, pathogenDNA, humanityDNA);
        
        // Печатаем событие
        printEvent(event, day);
        
        // Обрабатываем событие
        if (event.type == EventType::DNAGrant) {
            if (event.playerIndex == 0) {
                pathogenDNA += event.dnaAmount;
                std::cout << "  -> Pathogen DNA: " << pathogenDNA << std::endl;
            } else {
                humanityDNA += event.dnaAmount;
                std::cout << "  -> Humanity DNA: " << humanityDNA << std::endl;
            }
        }
        
        // Выполняем шаг симуляции
        simulateDay(world);
        
        // Печатаем статистику каждые 10 дней
        if (day % 10 == 0 || day == 1) {
            printWorldStats(world);
        }
        
        std::cout << std::string(80, '-') << std::endl;
    }
    
    // Финальная статистика
    std::cout << std::endl << "=== ФИНАЛЬНАЯ СТАТИСТИКА ===" << std::endl;
    std::cout << "Pathogen DNA: " << pathogenDNA << std::endl;
    std::cout << "Humanity DNA: " << humanityDNA << std::endl;
    
    double totalAlive = 0, totalDead = 0;
    for (const auto& country : world.countries) {
        totalAlive += country.pop.susceptible + country.pop.exposed + 
                      country.pop.infected + country.pop.recovered;
        totalDead += country.pop.dead;
    }
    
    std::cout << "Всего живых: " << static_cast<int>(totalAlive) << std::endl;
    std::cout << "Всего умерших: " << static_cast<int>(totalDead) << std::endl;
    std::cout << "Вакцина готова: " << (world.vaccine.isReady ? "ДА" : "НЕТ") << std::endl;
    
    return 0;
}
