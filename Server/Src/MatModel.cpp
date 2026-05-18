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

    // Names match Interface/Src/Screen.cpp map country identifiers.
    connect("CHINA", "RUSSIA", 5000.0);
    connect("CHINA", "INDIA", 8000.0);
    connect("CHINA", "USA", 3000.0);
    connect("RUSSIA", "USA", 2000.0);
    connect("USA", "INDIA", 4000.0);
    connect("USA", "BRAZIL", 3500.0);
    connect("INDIA", "BRAZIL", 2500.0);
    connect("BRAZIL", "S AFRICA", 1500.0);
    connect("S AFRICA", "CHINA", 2000.0);
    connect("CANADA", "USA", 4500.0);
    connect("MEXICO", "USA", 5000.0);
    connect("UK", "W EUROPE", 4500.0);
    connect("W EUROPE", "SCANDINAVIA", 2500.0);
    connect("W EUROPE", "TURKEY", 3200.0);
    connect("TURKEY", "MIDDLE EAST", 2800.0);
    connect("MIDDLE EAST", "INDIA", 3500.0);
    connect("MIDDLE EAST", "N AFRICA", 2500.0);
    connect("N AFRICA", "M AFRICA", 1800.0);
    connect("M AFRICA", "S AFRICA", 1800.0);
    connect("AUSTRALIA", "OCEANIA", 2200.0);
    connect("AUSTRALIA", "NEW ZELAND", 1800.0);
    connect("JAPAN", "CHINA", 3500.0);
    connect("MONGOLIA", "CHINA", 1200.0);
    connect("KAZAKHSTAN", "RUSSIA", 1600.0);
    connect("UKRAINE", "RUSSIA", 1500.0);
    connect("BELARUS", "RUSSIA", 1200.0);
    connect("GREENLAND", "ICELAND", 800.0);
    connect("ICELAND", "UK", 900.0);
    connect("N SOUTH AMERICA", "BRAZIL", 1800.0);
    connect("SW SOUTH AMERICA", "BRAZIL", 1800.0);
    connect("MADAGASCAR", "S AFRICA", 700.0);

    return connections;
}

// ---------- Инициализация мира (сокращённая) ----------
World initializeWorld() {
    World world;

    struct RawCountry {
        std::string name;
        int med, clim, urb, gov;
        double popM;
    };

    // Временный короткий список для отладки
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
    world.virus.incubationPeriod = 5.0;    // 5 дней инкубации
    world.virus.infectiousPeriod = 14.0;   // 14 дней заразности
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

// ========== МАТЕМАТИЧЕСКАЯ МОДЕЛЬ РАСПРОСТРАНЕНИЯ ВИРУСА ==========

/**
 * Обновление прогресса вакцины
 * Скорость разработки зависит от:
 * - Вовлеченности ученых (humanity.scientistCommitment)
 * - Уровня медицины в каждой стране
 * - Сложности разработки вакцины (virus.vaccineDifficulty)
 */
void updateVaccineProgress(World& world) {
    if (world.vaccine.isReady) return;

    // Базовая скорость разработки
    double baseSpeed = 0.1; // процентов в день

    // Глобальный вклад ученых
    double scientistFactor = world.humanity.scientistCommitment;

    // Средний уровень медицины в мире
    double avgMedicine = 0.0;
    for (const auto& country : world.countries) {
        avgMedicine += country.params.medicine / 5.0;
    }
    avgMedicine /= world.countries.size();

    // Сложность разработки снижает скорость
    double difficultyFactor = 1.0 - world.virus.vaccineDifficulty * 0.5;

    // Итоговый прогресс за день
    double dailyProgress = baseSpeed * scientistFactor * avgMedicine * difficultyFactor;

    // Дополнительный бонус от осведомленности (больше знают -> больше финансирования)
    dailyProgress *= (1.0 + world.humanity.awareness * 0.5);

    world.vaccine.updateProgress(dailyProgress);
}

/**
 * Обновление осведомленности человечества
 * Осведомленность растет с количеством зараженных и умерших
 */
void updateAwareness(World& world) {
    double totalInfected = 0.0;
    double totalDead = 0.0;
    double totalPopulation = 0.0;

    for (const auto& country : world.countries) {
        totalInfected += country.pop.infected + country.pop.exposed;
        totalDead += country.pop.dead;
        totalPopulation += country.pop.initial;
    }

    // Осведомленность зависит от процента зараженных и мертвых
    double infectionAwareness = totalInfected / totalPopulation * 10.0;
    double deathAwareness = totalDead / totalPopulation * 20.0;

    // Плавное увеличение осведомленности
    double targetAwareness = std::min(1.0, infectionAwareness + deathAwareness);
    double awarenessGrowth = (targetAwareness - world.humanity.awareness) * 0.1;

    world.humanity.awareness += awarenessGrowth;
}

/**
 * Расчет распространения между странами через транспортные связи
 */
void propagateBetweenCountries(World& world) {
    const double transmissionProbability = 0.001; // Вероятность передачи на одного путешественника

    for (auto& conn : world.connections) {
        if (!conn.isActive) continue; // Границы закрыты

        Country& fromCountry = world.countries[conn.from];
        Country& toCountry = world.countries[conn.to];

        if (fromCountry.bordersClosed || toCountry.bordersClosed) {
            conn.isActive = false;
            continue;
        }

        // Объем транспорта корректируется урбанизацией
        double transportFactor = (fromCountry.params.getTransportFactor() +
                                  toCountry.params.getTransportFactor()) / 2.0;
        double effectiveVolume = conn.transportVolume * transportFactor;

        // Доля зараженных в исходной стране
        double infectedFraction = fromCountry.pop.infected / fromCountry.pop.initial;

        // Количество зараженных путешественников
        double infectedTravelers = effectiveVolume * infectedFraction * transmissionProbability;

        // Добавляем экспонированных в целевую страну
        if (infectedTravelers > 0.1) { // Минимальный порог
            toCountry.pop.exposed += infectedTravelers;
        }
    }
}

/**
 * Основная функция обновления эпидемиологической модели (SEIR)
 * S - Susceptible (восприимчивые)
 * E - Exposed (зараженные, но не заразные)
 * I - Infected (зараженные и заразные)
 * R - Recovered (выздоровевшие/вакцинированные)
 * D - Dead (умершие)
 */
void updateEpidemicModel(World& world) {
    const auto& virus = world.virus;
    const auto& vaccine = world.vaccine;

    // Коэффициенты перехода между компартментами
    double sigma = 1.0 / virus.incubationPeriod;  // E -> I (инкубационный период)
    double gamma = 1.0 / virus.infectiousPeriod;  // I -> R/D (период заразности)

    for (auto& country : world.countries) {
        Population& pop = country.pop;
        double N = pop.initial;

        if (N <= 0) continue;

        double S = pop.susceptible;
        double E = pop.exposed;
        double I = pop.infected;
        double R = pop.recovered;
        double D = pop.dead;

        // --- Локальное распространение внутри страны ---

        // Модификатор от климата
        double climateMult = virus.getClimateModifier(country.params.climate);

        // Модификатор от урбанизации (более городские = быстрее распространение)
        double urbanizationFactor = country.params.getTransportFactor();

        // Эффективный коэффициент заражения
        double effectiveBeta = virus.infectivity * climateMult * urbanizationFactor;

        // Новые экспонированные (S -> E)
        double newExposed = effectiveBeta * (S / N) * I;
        if (newExposed > S) newExposed = S;

        // Переход из экспонированных в заразные (E -> I)
        double becomingInfectious = sigma * E;

        // Выздоровления и смерти (I -> R/D)
        double medicineFactor = 1.0 + country.params.medicine * 0.3;
        double effectiveLethality = virus.lethality / medicineFactor;
        double effectiveRecovery = gamma * (1.0 - effectiveLethality);
        double effectiveDeath = gamma * effectiveLethality;

        double newRecovered = effectiveRecovery * I;
        double newDeaths = effectiveDeath * I;

        // --- Вакцинация (если готова) ---
        double newVaccinated = 0.0;
        if (vaccine.isReady) {
            double vaccRate = country.params.getVaccineSpeed() * vaccine.spreadRate;
            vaccRate *= vaccine.efficacy; // Учитываем эффективность вакцины
            newVaccinated = vaccRate * S;
            if (newVaccinated > S) newVaccinated = S;
        }

        // --- Обновление компартментов ---
        S = S - newExposed - newVaccinated;
        E = E + newExposed - becomingInfectious;
        I = I + becomingInfectious - newRecovered - newDeaths;
        R = R + newRecovered + newVaccinated;
        D = D + newDeaths;

        // Ограничения
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
    // 1. Обновление осведомленности
    updateAwareness(world);

    // 2. Проверка закрытия границ
    world.checkBorderClosures();

    // 3. Распространение между странами
    propagateBetweenCountries(world);

    // 4. Обновление эпидемиологической модели
    updateEpidemicModel(world);

    // 5. Прогресс вакцины
    updateVaccineProgress(world);
}

// ---------- Симуляция ----------
#if defined(MATMODEL_STANDALONE) && MATMODEL_STANDALONE
int main() {
    World world = initializeWorld();

    // Начальная инфекция в Китае (добавляем зараженных)
    for (auto& c : world.countries) {
        if (c.name == "Китай") {
            c.pop.infected = 1000;
            c.pop.susceptible -= 1000;
        }
    }

    int totalDays = 365;
    int printStep = 30;

    std::cout << std::fixed << std::setprecision(1);

    for (int day = 1; day <= totalDays; ++day) {
        // Симуляция одного дня
        simulateDay(world);

        // Вывод статистики
        if (day % printStep == 0 || day == 1) {
            std::cout << "\n--- Day " << day << " ---\n";
            std::cout << std::left << std::setw(20) << "Country"
                      << std::right << std::setw(12) << "Suscept."
                      << std::setw(12) << "Exposed"
                      << std::setw(12) << "Infected"
                      << std::setw(12) << "Dead"
                      << std::setw(12) << "Recovered"
                      << std::setw(12) << "S %" << "\n";
            std::cout << std::string(92, '-') << "\n";

            for (const auto& c : world.countries) {
                double N = c.pop.initial;
                double S = c.pop.susceptible;
                std::cout << std::left << std::setw(20) << c.name
                          << std::right << std::setw(12) << c.pop.susceptible
                          << std::setw(12) << c.pop.exposed
                          << std::setw(12) << c.pop.infected
                          << std::setw(12) << c.pop.dead
                          << std::setw(12) << c.pop.recovered
                          << std::setw(11) << (S / N * 100.0) << "%\n";
            }

            // Глобальная статистика
            double totalInfected = 0, totalDead = 0, totalRecovered = 0;
            for (const auto& c : world.countries) {
                totalInfected += c.pop.infected + c.pop.exposed;
                totalDead += c.pop.dead;
                totalRecovered += c.pop.recovered;
            }

            std::cout << "\nGlobal stats:\n";
            std::cout << "  Total Infected: " << totalInfected << "\n";
            std::cout << "  Total Dead: " << totalDead << "\n";
            std::cout << "  Total Recovered: " << totalRecovered << "\n";
            std::cout << "  Vaccine Progress: " << world.vaccine.progress << "%\n";
            std::cout << "  Humanity Awareness: " << (world.humanity.awareness * 100) << "%\n";

            // Статус границ
            int closedCount = 0;
            for (const auto& conn : world.connections) {
                if (!conn.isActive) closedCount++;
            }
            std::cout << "  Closed Borders: " << closedCount << "/" << world.connections.size() << "\n";
        }
    }

    return 0;
}

#endif // MATMODEL_STANDALONE

} // namespace plague