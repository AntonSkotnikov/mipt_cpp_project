#include "EventGenerator.hpp"
#include <sstream>
#include <algorithm>

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

int EventGenerator::selectRandomCountry(const World& world) {
    if (world.countries.empty()) return -1;
    return randomInt(0, static_cast<int>(world.countries.size()) - 1);
}

int EventGenerator::selectRandomUninfectedCountry(const World& world) {
    std::vector<int> uninfectedIndices;
    
    for (size_t i = 0; i < world.countries.size(); ++i) {
        const auto& country = world.countries[i];
        // Страна считается незараженной, если нет активных зараженных
        if (country.pop.infected < 1.0 && country.pop.exposed < 1.0) {
            uninfectedIndices.push_back(static_cast<int>(i));
        }
    }
    
    if (uninfectedIndices.empty()) return -1;
    
    size_t randomIdx = static_cast<size_t>(randomInt(0, static_cast<int>(uninfectedIndices.size()) - 1));
    return uninfectedIndices[randomIdx];
}

// ========== ГЛАВНАЯ ФУНКЦИЯ ГЕНЕРАЦИИ ==========
EventResult EventGenerator::generateEvent(World& world, int day,
                                          int pathogenDNA, int humanityDNA) {
    EventResult result;
    
    // Приоритет событий:
    // 1. Первое заражение (если еще не было ни одного зараженного в мире)
    // 2. Транспорт (всегда происходит с некоторой вероятностью)
    // 3. Закрытие границ (зависит от ситуации)
    // 4. Начисление DNA (периодически)
    
    // Проверяем, есть ли вообще зараженные в мире
    bool hasInfection = false;
    for (const auto& country : world.countries) {
        if (country.pop.infected > 0 || country.pop.exposed > 0) {
            hasInfection = true;
            break;
        }
    }
    
    // Если инфекции еще нет, пытаемся создать первое заражение
    if (!hasInfection) {
        result = tryFirstInfection(world);
        if (result.type != EventType::None) {
            return result;
        }
    }
    
    // Всегда пробуем транспорт (это основной механизм распространения)
    result = tryTransportMovement(world);
    if (result.type != EventType::None) {
        return result;
    }
    
    // Пробуем закрытие границ
    result = tryBorderClosure(world);
    if (result.type != EventType::None) {
        return result;
    }
    
    // Пробуем начисление DNA
    result = tryDNAGrant(world, day, pathogenDNA, humanityDNA);
    if (result.type != EventType::None) {
        return result;
    }
    
    // Никакое событие не произошло
    return EventResult();
}

// ========== ПЕРВОЕ ЗАРАЖЕНИЕ ==========
EventResult EventGenerator::tryFirstInfection(World& world) {
    EventResult result;
    
    // Выбираем случайную незараженную страну
    int countryIdx = selectRandomUninfectedCountry(world);
    if (countryIdx < 0) {
        return result; // Все страны уже заражены
    }
    
    // Шанс первого заражения (можно настроить)
    // В реальной игре это происходит по выбору игрока, но здесь симулируем
    double infectionChance = 0.1; // 10% шанс каждый день пока нет инфекции
    
    if (randomDouble() > infectionChance) {
        return result;
    }
    
    // Количество зараженных зависит от параметров
    int infectedCount = params_.baseFirstInfectionCount + 
                        randomInt(-20, 50);
    infectedCount = std::max(10, infectedCount);
    
    // Применяем заражение
    Country& country = world.countries[static_cast<size_t>(countryIdx)];
    if (country.pop.susceptible >= infectedCount) {
        country.pop.susceptible -= infectedCount;
        country.pop.exposed += infectedCount; // Начинаем с экспонированных
        
        result.type = EventType::FirstInfection;
        result.countryIndex = static_cast<size_t>(countryIdx);
        result.infectedCount = infectedCount;
        
        std::ostringstream ss;
        ss << "Первое заражение в стране " << country.name 
           << "! " << infectedCount << " человек инфицировано.";
        result.description = ss.str();
    }
    
    return result;
}

// ========== ТРАНСПОРТ ==========
EventResult EventGenerator::tryTransportMovement(World& world) {
    EventResult result;
    
    if (world.connections.empty()) {
        return result;
    }
    
    // Выбираем случайное активное соединение
    std::vector<size_t> activeConnections;
    for (size_t i = 0; i < world.connections.size(); ++i) {
        if (world.connections[i].isActive) {
            activeConnections.push_back(i);
        }
    }
    
    if (activeConnections.empty()) {
        return result; // Все границы закрыты
    }
    
    size_t connIdx = activeConnections[static_cast<size_t>(
        randomInt(0, static_cast<int>(activeConnections.size()) - 1))];
    
    CountryConnection& conn = world.connections[connIdx];
    Country& fromCountry = world.countries[conn.from];
    Country& toCountry = world.countries[conn.to];
    
    // Базовый объем транспорта
    double transportFactor = (fromCountry.params.getTransportFactor() + 
                              toCountry.params.getTransportFactor()) / 2.0;
    double effectiveVolume = conn.transportVolume * transportFactor;
    
    // Доля зараженных в исходной стране
    double infectedFraction = fromCountry.pop.infected / 
                              std::max(1.0, fromCountry.pop.initial);
    
    // Количество потенциально зараженных путешественников
    double infectedTravelers = effectiveVolume * infectedFraction;
    
    // Шанс перевозки зараженных зависит от заразности вируса
    double transportChance = params_.baseInfectedTransportChance + 
                             world.virus.infectivity * params_.infectivityModifier;
    transportChance = std::min(0.95, transportChance);
    
    result.type = EventType::TransportMovement;
    result.fromCountry = conn.from;
    result.toCountry = conn.to;
    
    std::ostringstream ss;
    ss << "Транспорт из " << fromCountry.name << " в " << toCountry.name;
    
    // Проверяем, перевезли ли зараженных
    if (infectedTravelers >= 0.5 && randomDouble() < transportChance) {
        int infectedCount = static_cast<int>(std::max(1.0, infectedTravelers));
        
        // Добавляем экспонированных в целевую страну
        toCountry.pop.exposed += infectedCount;
        
        result.transportedInfected = true;
        result.infectedTransported = infectedCount;
        
        ss << " - перевезено " << infectedCount << " зараженных!";
    } else {
        ss << " - без зараженных.";
    }
    
    result.description = ss.str();
    
    return result;
}

// ========== ЗАКРЫТИЕ ГРАНИЦ ==========
EventResult EventGenerator::tryBorderClosure(World& world) {
    EventResult result;
    
    // Базовый шанс закрытия границ
    double closeChance = params_.baseBorderCloseChance;
    
    for (size_t i = 0; i < world.countries.size(); ++i) {
        Country& country = world.countries[i];
        
        // Если границы уже закрыты, пропускаем
        if (country.bordersClosed) {
            continue;
        }
        
        // Процент зараженных в стране
        double infectionRate = country.pop.infected / 
                               std::max(1.0, country.pop.initial);
        
        // Вычисляем вероятность закрытия границ
        double countryCloseChance = closeChance +
            infectionRate * 10.0 * params_.awarenessModifier +
            country.params.governmentReaction * params_.governmentReactionModifier +
            world.humanity.awareness * params_.awarenessModifier;
        
        // Модификатор от урбанизации (более городские страны быстрее реагируют)
        countryCloseChance *= (0.8 + country.params.urbanization * 0.1);
        
        if (randomDouble() < countryCloseChance) {
            // Закрываем все границы страны
            world.closeAllBorders(i);
            
            result.type = EventType::BorderClosure;
            result.closedCountry = i;
            
            std::ostringstream ss;
            ss << "Страна " << country.name << " закрыла границы! "
               << "Реакция властей: " << country.params.governmentReaction
               << ", Осведомленность: " << (world.humanity.awareness * 100) << "%";
            result.description = ss.str();
            
            return result;
        }
    }
    
    return result;
}

// ========== НАЧИСЛЕНИЕ DNA ==========
EventResult EventGenerator::tryDNAGrant(World& world, int day, 
                                        int pathogenDNA, int humanityDNA) {
    EventResult result;
    
    // Проверяем интервал
    if (day - lastDNAGrantDay_ < params_.dnaGrantInterval) {
        return result;
    }
    
    // Проверяем шанс
    if (randomDouble() > params_.dnaGrantChance) {
        return result;
    }
    
    // Обновляем день последнего начисления
    lastDNAGrantDay_ = day;
    
    // Выбираем игрока случайно или на основе достижений
    int playerIndex = randomInt(0, 1);
    int dnaAmount = params_.baseDNAAmount + randomInt(0, 3);
    
    // Бонусы за достижения
    if (playerIndex == 0) { // Pathogen
        // Считаем общее количество зараженных
        double totalInfected = 0;
        for (const auto& country : world.countries) {
            totalInfected += country.pop.infected + country.pop.exposed;
        }
        
        // Бонус за массовое заражение
        if (totalInfected > 1000000) {
            dnaAmount += params_.pathogenDNAFromInfections;
        }
        
        result.playerIndex = 0;
    } else { // Humanity
        // Бонус за прогресс вакцины
        if (world.vaccine.progress > 50.0) {
            dnaAmount += params_.humanityDNAFromVaccine;
        }
        
        // Дополнительный бонус за готовую вакцину
        if (world.vaccine.isReady) {
            dnaAmount += 3;
        }
        
        result.playerIndex = 1;
    }
    
    result.type = EventType::DNAGrant;
    result.dnaAmount = dnaAmount;
    
    std::ostringstream ss;
    ss << "Игрок " << (playerIndex == 0 ? "Pathogen" : "Humanity")
       << " получил " << dnaAmount << " DNA!";
    result.description = ss.str();
    
    return result;
}

} // namespace plague
