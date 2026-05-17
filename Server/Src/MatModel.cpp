#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>
#include <cmath>
#include <random>

namespace plague {

// ========== ПАРАМЕТРЫ СТРАНЫ ==========
struct CountryParams {
    int medicine;           // 1-5: уровень развития медицины
    int climate;            // 1-5: климат (1=холодный, 5=тропический)
    int urbanization;       // 1-5: степень урбанизации
    int governmentReaction; // 1-5: скорость реакции властей

    // Вычисляемые параметры
    double getScienceFactor() const {
        // Количество ученых пропорционально уровню медицины
        return medicine / 5.0;
    }

    double getVaccineSpeed() const {
        // Скорость распространения вакцины зависит от медицины
        return 0.5 + (medicine / 5.0) * 0.5;
    }

    double getTransportFactor() const {
        // Количество транспорта зависит от урбанизации
        return 0.2 + (urbanization / 5.0) * 0.8;
    }

    double getBorderCloseSpeed() const {
        // Скорость закрытия границ зависит от реакции властей
        return governmentReaction / 5.0;
    }
};

// ========== НАСЕЛЕНИЕ (SEIR модель) ==========
struct Population {
    double initial;         // Стартовое количество
    double susceptible;     // Восприимчивые (S)
    double exposed;         // Зараженные но не заразные (E)
    double infected;        // Зараженные и заразные (I)
    double recovered;       // Вылеченные/Вакцинированные (R)
    double dead;            // Умершие (D)

    Population(double total = 0.0)
        : initial(total), susceptible(total), exposed(0),
          infected(0), recovered(0), dead(0) {}

    double alive() const {
        return susceptible + exposed + infected + recovered;
    }
};

// ========== СТРАНА ==========
struct Country {
    std::string name;
    CountryParams params;
    Population pop;
    double borderOpenness;  // 0.0 = полностью закрыта, 1.0 = полностью открыта
    bool bordersClosed;     // Флаг полного закрытия границ

    Country() : borderOpenness(1.0), bordersClosed(false) {}
};

// ========== ВИРУС ==========
struct Virus {
    double infectivity;         // Заразность (базовый коэффициент передачи)
    double lethality;           // Летальность (базовый процент смертности)
    double vaccineDifficulty;   // Сложность изготовления вакцины (0-1)
    double incubationPeriod;    // Период инкубации (дни)
    double infectiousPeriod;    // Период заразности (дни)
    std::vector<double> climateModifiers; // Модификаторы для разных климатов

    double getClimateModifier(int climateLevel) const {
        if (climateLevel < 1 || climateLevel > 5) return 1.0;
        return climateModifiers[climateLevel - 1];
    }
};

// ========== ЧЕЛОВЕЧЕСТВО ==========
struct Humanity {
    double scientistCommitment;     // Вовлеченность ученых (0-1)
    double awareness;               // Осведомленность (0-1)
    double developmentDifficultyMod;// Сложности разработки вакцины

    double getGlobalScientists() const {
        // Общее количество ученых в мире
        return scientistCommitment * 10000.0; // условных единиц
    }

    double getBorderCloseThreshold() const {
        // Порог осведомленности для закрытия границ
        return 0.3 - awareness * 0.2; // чем выше осведомленность, тем раньше закрывают
    }
};

// ========== ВАКЦИНА ==========
struct Vaccine {
    double progress;          // Степень готовности (0-100%)
    double spreadRate;        // Скорость распространения
    bool isReady;             // Готова ли к применению
    double efficacy;          // Эффективность (0-1)

    Vaccine() : progress(0), spreadRate(0.005), isReady(false), efficacy(0.95) {}

    void updateProgress(double delta) {
        progress += delta;
        if (progress >= 100.0) {
            progress = 100.0;
            isReady = true;
        }
    }
};

// ========== СВЯЗЬ МЕЖДУ СТРАНАМИ ==========
struct CountryConnection {
    size_t from;              // Индекс страны 1
    size_t to;                // Индекс страны 2
    double transportVolume;   // Объем транспорта (людей в день)
    bool isActive;            // Активна ли связь (не закрыты ли границы)

    CountryConnection(size_t f, size_t t, double volume)
        : from(f), to(t), transportVolume(volume), isActive(true) {}
};

// ========== МИР ==========
class World {
public:
    std::vector<Country> countries;
    std::vector<CountryConnection> connections;
    Virus virus;
    Humanity humanity;
    Vaccine vaccine;

    // Получить индекс страны по имени
    int getCountryIndex(const std::string& name) const {
        for (size_t i = 0; i < countries.size(); ++i) {
            if (countries[i].name == name) return (int)i;
        }
        return -1;
    }

    // Закрыть границы между двумя странами
    void closeBorder(size_t i, size_t j) {
        for (auto& conn : connections) {
            if ((conn.from == i && conn.to == j) ||
                (conn.from == j && conn.to == i)) {
                conn.isActive = false;
            }
        }
        if (i < countries.size()) countries[i].bordersClosed = true;
        if (j < countries.size()) countries[j].bordersClosed = true;
    }

    // Закрыть все границы страны
    void closeAllBorders(size_t countryIdx) {
        if (countryIdx >= countries.size()) return;
        countries[countryIdx].bordersClosed = true;
        countries[countryIdx].borderOpenness = 0.0;

        for (auto& conn : connections) {
            if (conn.from == countryIdx || conn.to == countryIdx) {
                conn.isActive = false;
            }
        }
    }

    // Проверить и автоматически закрыть границы при высокой осведомленности
    void checkBorderClosures() {
        double threshold = humanity.getBorderCloseThreshold();

        for (size_t i = 0; i < countries.size(); ++i) {
            Country& country = countries[i];

            // Процент зараженных в стране
            double infectionRate = country.pop.infected / country.pop.initial;

            // Глобальная осведомленность о вирусе
            double globalAwareness = humanity.awareness;

            // Вероятность закрытия границ
            double closeProbability = (infectionRate + globalAwareness) *
                                     country.params.getBorderCloseSpeed();

            if (closeProbability > threshold && !country.bordersClosed) {
                closeAllBorders(i);
            }
        }
    }
};

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

    // Транспортные связи между странами (объем транспорта зависит от урбанизации)
    connect("Китай", "Россия", 5000.0);
    connect("Китай", "Индия", 8000.0);
    connect("Китай", "США", 3000.0);
    connect("Россия", "США", 2000.0);
    connect("США", "Индия", 4000.0);
    connect("США", "Бразилия", 3500.0);
    connect("Индия", "Бразилия", 2500.0);
    connect("Бразилия", "Южная Африка", 1500.0);
    connect("Южная Африка", "Китай", 2000.0);

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
        {"Китай",        3, 3, 5, 5, 1400.0},
        {"США",          5, 3, 5, 3, 331.0},
        {"Россия",       3, 3, 3, 3, 144.0},
        {"Индия",        2, 5, 4, 2, 1400.0},
        {"Бразилия",     2, 5, 4, 2, 213.0},
        {"Южная Африка", 3, 4, 3, 3, 60.0},
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
    world.virus.lethality = 0.02;
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
#endif
}