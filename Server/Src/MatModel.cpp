#include <iostream>
#include <vector>
#include <string>
#include <iomanip>
#include <algorithm>

struct CountryParams {
    int medicine;
    int climate;
    int urbanization;
    int governmentReaction;
};

struct Population {
    double initial;
    double infected;
    double recovered;
    double dead;

    double susceptible() const {
        return initial - infected - recovered - dead;
    }

    Population(double total = 0.0)
        : initial(total), infected(0), recovered(0), dead(0) {}
};

struct Country {
    std::string name;
    CountryParams params;
    Population pop;
};

struct Virus {
    double infectivity;
    double lethality;
    double vaccineDifficulty;
    std::vector<double> climateModifiers;
};

struct Humanity {
    double scientistCommitment;
    double awareness;
    double developmentDifficultyMod;
};

struct Vaccine {
    double progress;
    double spreadRate;
    bool isReady;
};

class World {
public:
    std::vector<Country> countries;
    Virus virus;
    Humanity humanity;
    Vaccine vaccine;
    std::vector<std::vector<bool>> connection;

    void closeBorder(size_t i, size_t j) {
        if (i < connection.size() && j < connection.size()) {
            connection[i][j] = false;
            connection[j][i] = false;
        }
    }
};

// ---------- Инициализация связей (сокращённая) ----------
std::vector<std::vector<bool>> buildInitialConnections(const std::vector<Country>& countries) {
    size_t n = countries.size();
    std::vector<std::vector<bool>> mat(n, std::vector<bool>(n, false));
    auto idx = [&](const std::string& name) -> int {
        for (size_t i = 0; i < n; ++i)
            if (countries[i].name == name) return (int)i;
        return -1;
    };
    auto connect = [&](const std::string& a, const std::string& b) {
        int ia = idx(a), ib = idx(b);
        if (ia >= 0 && ib >= 0) mat[ia][ib] = mat[ib][ia] = true;
    };

    // Только для нашего короткого списка
    connect("Китай", "Россия");
    connect("Китай", "Индия");
    connect("Китай", "США");
    connect("Россия", "США");
    connect("США", "Индия");
    connect("США", "Бразилия");
    connect("Индия", "Бразилия");
    connect("Бразилия", "Южная Африка");
    connect("Южная Африка", "Китай");
    return mat;
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

    world.connection = buildInitialConnections(world.countries);

    world.virus.infectivity = 0.3;
    world.virus.lethality = 0.02;
    world.virus.vaccineDifficulty = 0.5;
    world.virus.climateModifiers = {1.0, 1.0, 1.0, 1.0, 1.0};

    world.humanity.scientistCommitment = 0.1;
    world.humanity.awareness = 0.0;
    world.humanity.developmentDifficultyMod = 0.0;

    world.vaccine.progress = 0.0;
    world.vaccine.spreadRate = 0.005;
    world.vaccine.isReady = false;

    return world;
}

// ---------- Обновление населения (без изменений) ----------
void updatePopulation(World& world) {
    const auto& virus = world.virus;
    const auto& vaccine = world.vaccine;

    for (auto& country : world.countries) {
        Population& pop = country.pop;
        double N = pop.initial;
        double S = pop.susceptible();
        double I = pop.infected;
        double R = pop.recovered;
        double D = pop.dead;

        if (N <= 0) continue;

        int climateIdx = country.params.climate - 1;
        double climateMult = virus.climateModifiers[climateIdx];
        double urbanizationFactor = country.params.urbanization / 5.0;
        double effectiveBeta = virus.infectivity * climateMult * urbanizationFactor;

        double newInfections = effectiveBeta * (S / N) * I;
        if (newInfections > S) newInfections = S;

        double medicineFactor = 1.0 + country.params.medicine * 0.5;
        double effectiveLethality = virus.lethality / medicineFactor;
        double newDeaths = effectiveLethality * I;
        if (newDeaths > I) newDeaths = I;

        double newVaccinated = 0.0;
        if (vaccine.isReady) {
            double vaccRate = (country.params.medicine / 5.0) * vaccine.spreadRate;
            newVaccinated = vaccRate * S;
            if (newVaccinated > S) newVaccinated = S;
        }

        S = S - newInfections - newVaccinated;
        I = I + newInfections - newDeaths;
        R = R + newVaccinated;
        D = D + newDeaths;

        if (S < 0) S = 0;
        if (I < 0) I = 0;
        if (R < 0) R = 0;
        if (D < 0) D = 0;

        pop.infected = I;
        pop.recovered = R;
        pop.dead = D;
    }
}

// ---------- Симуляция ----------
int main() {
    World world = initializeWorld();

    // Начальная инфекция в Китае
    for (auto& c : world.countries) {
        if (c.name == "Китай") c.pop.infected = 1000;
    }

    int totalDays = 365;
    int printStep = 30;

    std::cout << std::fixed << std::setprecision(1);

    for (int day = 1; day <= totalDays; ++day) {
        if (day == 100) {
            world.vaccine.isReady = true;
            std::cout << "\n>>> Vaccine ready on day " << day << " <<<\n";
        }

        updatePopulation(world);

        if (day % printStep == 0 || day == 1) {
            std::cout << "\n--- Day " << day << " ---\n";
            std::cout << std::left << std::setw(20) << "Country"
                      << std::right << std::setw(14) << "Infected"
                      << std::setw(14) << "Dead"
                      << std::setw(14) << "Vaccinated"
                      << std::setw(14) << "Susceptible %" << "\n";
            std::cout << std::string(76, '-') << "\n";

            for (const auto& c : world.countries) {
                double N = c.pop.initial;
                double S = c.pop.susceptible();
                std::cout << std::left << std::setw(20) << c.name
                          << std::right << std::setw(14) << c.pop.infected
                          << std::setw(14) << c.pop.dead
                          << std::setw(14) << c.pop.recovered
                          << std::setw(13) << (S / N * 100.0) << "%\n";
            }
        }
    }

    return 0;
}