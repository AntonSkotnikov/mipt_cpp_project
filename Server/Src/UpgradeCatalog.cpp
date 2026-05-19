#include "UpgradeCatalog.hpp"

namespace plague {

namespace {

// Pathogen Upgrades - Transmission (Пути передачи)
const std::vector<UpgradeDefinition> pathogenUpgrades = {
    // Airborne Transmission (Воздушная передача)
    {"air_1", UpgradeCategory::Transmission, "Air I", 5, "Improves airborne transmission slightly.\nIncreases infectivity by 10%.", {}, 0.10, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"air_2", UpgradeCategory::Transmission, "Air II", 10, "A stronger airborne transmission upgrade.\nRequires Air I. Increases infectivity by 15%.", {"air_1"}, 0.15, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"air_3", UpgradeCategory::Transmission, "Air III", 20, "Maximum airborne transmission.\nRequires Air II. Increases infectivity by 20%.", {"air_2"}, 0.20, 0.0, 0.0, 0.0, 0.0, 0.0},

    // Water Transmission (Водная передача)
    {"water_1", UpgradeCategory::Transmission, "Water I", 6, "Improves water transmission slightly.\nIncreases infectivity by 12%.", {}, 0.12, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"water_2", UpgradeCategory::Transmission, "Water II", 12, "A stronger water transmission upgrade.\nRequires Water I. Increases infectivity by 18%.", {"water_1"}, 0.18, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"water_3", UpgradeCategory::Transmission, "Water III", 22, "Maximum water transmission.\nRequires Water II. Increases infectivity by 25%.", {"water_2"}, 0.25, 0.0, 0.0, 0.0, 0.0, 0.0},

    // Extreme Bioaerosol (Экстремальный биоаэрозоль)
    {"extreme_bioaerosol", UpgradeCategory::Transmission, "Extreme Bioaerosol", 35, "Advanced airborne mutation.\nRequires Air III. Greatly increases global infectivity.", {"air_3"}, 0.35, 0.0, 0.0, 0.0, 0.0, 0.0},

    // Symptoms - Mild (Лёгкие симптомы)
    {"coughing", UpgradeCategory::Abilities, "Coughing", 4, "Causes mild coughing.\nSlightly increases infectivity and visibility.", {}, 0.05, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"sneezing", UpgradeCategory::Abilities, "Sneezing", 6, "Causes sneezing fits.\nIncreases infectivity by 8%.", {"coughing"}, 0.08, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"nausea", UpgradeCategory::Abilities, "Nausea", 5, "Causes nausea and vomiting.\nIncreases infectivity in poor regions.", {}, 0.07, 0.0, 0.0, 0.0, 0.0, 0.0},

    // Symptoms - Severe (Тяжёлые симптомы)
    {"pneumonia", UpgradeCategory::Abilities, "Pneumonia", 15, "Causes severe pneumonia.\nSignificantly increases lethality by 5%.", {"sneezing"}, 0.0, 0.05, 0.0, 0.0, 0.0, 0.0},
    {"organ_failure", UpgradeCategory::Abilities, "Organ Failure", 25, "Causes multiple organ failure.\nGreatly increases lethality by 10%.", {"pneumonia"}, 0.0, 0.10, 0.0, 0.0, 0.0, 0.0},
    {"hemorrhagic_shock", UpgradeCategory::Abilities, "Hemorrhagic Shock", 35, "Causes internal bleeding.\nMassively increases lethality by 15%.", {"organ_failure"}, 0.0, 0.15, 0.0, 0.0, 0.0, 0.0},

    // Symptoms - Neural (Нейронные симптомы)
    {"headache", UpgradeCategory::Abilities, "Headache", 7, "Causes chronic headaches.\nSlightly increases visibility.", {}, 0.03, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"insomnia", UpgradeCategory::Abilities, "Insomnia", 9, "Causes sleep disorders.\nIncreases visibility and stress.", {"headache"}, 0.04, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"paranoia", UpgradeCategory::Abilities, "Paranoia", 18, "Causes extreme paranoia.\nIncreases visibility significantly.", {"insomnia"}, 0.05, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"madness", UpgradeCategory::Abilities, "Madness", 30, "Causes complete mental breakdown.\nMaximum visibility but high lethality.", {"paranoia"}, 0.0, 0.08, 0.0, 0.0, 0.0, 0.0},

    // Environmental Resistance (Устойчивость к окружающей среде)
    {"cold_resistance_1", UpgradeCategory::Abilities, "Cold Resistance I", 8, "Improves survival in cold regions.\nRequired for cold climate spread.", {}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"cold_resistance_2", UpgradeCategory::Abilities, "Cold Resistance II", 15, "Better cold resistance.\nRequires Cold Resistance I.", {"cold_resistance_1"}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"heat_resistance_1", UpgradeCategory::Abilities, "Heat Resistance I", 8, "Improves survival in hot regions.\nRequired for hot climate spread.", {}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"heat_resistance_2", UpgradeCategory::Abilities, "Heat Resistance II", 15, "Better heat resistance.\nRequires Heat Resistance I.", {"heat_resistance_1"}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"drug_resistance_1", UpgradeCategory::Abilities, "Drug Resistance I", 12, "Improves survival against treatments.\nReduces cure speed by 10%.", {}, 0.0, 0.0, 0.10, 0.0, 0.0, 0.0},
    {"drug_resistance_2", UpgradeCategory::Abilities, "Drug Resistance II", 22, "Advanced drug resistance.\nRequires Drug Resistance I. Reduces cure speed by 20%.", {"drug_resistance_1"}, 0.0, 0.0, 0.20, 0.0, 0.0, 0.0},

    // Genetic Reshuffle (Генетическая перестройка)
    {"genetic_reshuffle_1", UpgradeCategory::Abilities, "Genetic Reshuffle I", 20, "Resets cure progress by 15%.\nOne-time use effect.", {}, 0.0, 0.0, 0.15, 0.0, 0.0, 0.0},
    {"genetic_reshuffle_2", UpgradeCategory::Abilities, "Genetic Reshuffle II", 35, "Resets cure progress by 30%.\nRequires Genetic Reshuffle I.", {"genetic_reshuffle_1"}, 0.0, 0.0, 0.30, 0.0, 0.0, 0.0},

    // Cure Delay (Задержка лечения)
    {"cure_delay", UpgradeCategory::Clinic, "Cure Delay", 10, "Slows research progress by 10%.\nPassive effect.", {}, 0.0, 0.0, 0.10, 0.0, 0.0, 0.0},
    {"clinic_overload", UpgradeCategory::Clinic, "Clinic Overload", 18, "Pressures medical systems.\nRequires Cure Delay. Slows cure by additional 15%.", {"cure_delay"}, 0.0, 0.0, 0.15, 0.0, 0.0, 0.0},
    {"research_noise", UpgradeCategory::Clinic, "Research Noise", 14, "Makes cure data less reliable.\nReduces vaccine effectiveness by 10%.", {}, 0.0, 0.0, 0.0, 0.0, 0.0, -0.10},
    {"data_corruption", UpgradeCategory::Clinic, "Data Corruption", 25, "Corrupts research databases.\nRequires Research Noise. Reduces vaccine effectiveness by 20%.", {"research_noise"}, 0.0, 0.0, 0.0, 0.0, 0.0, -0.20}
};

// Humanity Upgrades - Vaccine & Countermeasures
const std::vector<UpgradeDefinition> humanityUpgrades = {
    // Vaccine Development (Разработка вакцины)
    {"vaccine_research_1", UpgradeCategory::Clinic, "Vaccine Research I", 10, "Basic vaccine research program.\nIncreases cure speed by 15%.", {}, 0.0, 0.0, 0.0, 0.0, 0.15, 0.0},
    {"vaccine_research_2", UpgradeCategory::Clinic, "Vaccine Research II", 20, "Advanced vaccine research.\nRequires Vaccine Research I. Increases cure speed by 25%.", {"vaccine_research_1"}, 0.0, 0.0, 0.0, 0.0, 0.25, 0.0},
    {"vaccine_research_3", UpgradeCategory::Clinic, "Vaccine Research III", 35, "Cutting-edge vaccine development.\nRequires Vaccine Research II. Increases cure speed by 40%.", {"vaccine_research_2"}, 0.0, 0.0, 0.0, 0.0, 0.40, 0.0},

    {"vaccine_production_1", UpgradeCategory::Clinic, "Vaccine Production I", 15, "Mass production facilities.\nIncreases vaccine distribution by 20%.", {"vaccine_research_2"}, 0.0, 0.0, 0.0, 0.0, 0.20, 0.0},
    {"vaccine_production_2", UpgradeCategory::Clinic, "Vaccine Production II", 28, "Global distribution network.\nRequires Vaccine Production I. Increases distribution by 35%.", {"vaccine_production_1"}, 0.0, 0.0, 0.0, 0.0, 0.35, 0.0},

    // Quarantine Measures (Карантинные меры)
    {"quarantine_1", UpgradeCategory::Transmission, "Quarantine I", 12, "Local quarantine measures.\nReduces disease spread by 15% in affected regions.", {}, 0.0, 0.0, 0.0, 0.05, 0.0, 0.0},
    {"quarantine_2", UpgradeCategory::Transmission, "Quarantine II", 22, "National quarantine protocol.\nRequires Quarantine I. Reduces spread by 25%.", {"quarantine_1"}, 0.0, 0.0, 0.0, 0.10, 0.0, 0.0},
    {"quarantine_3", UpgradeCategory::Transmission, "Quarantine III", 38, "Global lockdown protocol.\nRequires Quarantine II. Reduces spread by 40%.", {"quarantine_2"}, 0.0, 0.0, 0.0, 0.15, 0.0, 0.0},

    // Travel Restrictions (Ограничения на поездки)
    {"travel_restrictions_1", UpgradeCategory::Transmission, "Travel Restrictions I", 10, "Domestic travel limits.\nReduces transport volume by 20%.", {}, 0.0, 0.0, 0.0, 0.03, 0.0, 0.0},
    {"travel_restrictions_2", UpgradeCategory::Transmission, "Travel Restrictions II", 18, "International flight bans.\nRequires Travel Restrictions I. Reduces transport by 35%.", {"travel_restrictions_1"}, 0.0, 0.0, 0.0, 0.06, 0.0, 0.0},
    {"travel_restrictions_3", UpgradeCategory::Transmission, "Travel Restrictions III", 30, "Complete border closure.\nRequires Travel Restrictions II. Reduces transport by 50%.", {"travel_restrictions_2"}, 0.0, 0.0, 0.0, 0.10, 0.0, 0.0},

    // Public Awareness (Информирование населения)
    {"public_awareness_1", UpgradeCategory::Abilities, "Public Awareness I", 8, "Health education campaign.\nReduces infection rate by 10%.", {}, 0.0, 0.0, 0.0, 0.10, 0.0, 0.0},
    {"public_awareness_2", UpgradeCategory::Abilities, "Public Awareness II", 15, "Media information blitz.\nRequires Public Awareness I. Reduces infection by 18%.", {"public_awareness_1"}, 0.0, 0.0, 0.0, 0.18, 0.0, 0.0},
    {"hygiene_campaign", UpgradeCategory::Abilities, "Hygiene Campaign", 12, "Promotes hygiene practices.\nReduces water transmission by 25%.", {"public_awareness_1"}, 0.0, 0.0, 0.0, 0.08, 0.0, 0.0},

    // Medical Infrastructure (Медицинская инфраструктура)
    {"hospital_beds", UpgradeCategory::Clinic, "Hospital Beds", 14, "Expands hospital capacity.\nReduces lethality by 10%.", {}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"ventilators", UpgradeCategory::Clinic, "Ventilators", 20, "Provides critical care equipment.\nRequires Hospital Beds. Reduces lethality by 18%.", {"hospital_beds"}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
    {"medical_supplies", UpgradeCategory::Clinic, "Medical Supplies", 16, "Stockpiles essential supplies.\nImproves treatment effectiveness by 15%.", {}, 0.0, 0.0, 0.0, 0.0, 0.0, 0.05},

    // Research Grants (Исследовательские гранты)
    {"research_grants", UpgradeCategory::Clinic, "Research Grants", 25, "Funds scientific research.\nIncreases all research speed by 20%.", {"vaccine_research_1"}, 0.0, 0.0, 0.0, 0.0, 0.20, 0.0},
    {"international_cooperation", UpgradeCategory::Clinic, "International Cooperation", 35, "Global research collaboration.\nRequires Research Grants. Increases research speed by 35%.", {"research_grants"}, 0.0, 0.0, 0.0, 0.0, 0.35, 0.05}
};

}  // namespace

const std::vector<UpgradeDefinition>& availableUpgradesFor(PlayerRole role) {
    return role == PlayerRole::Pathogen ? pathogenUpgrades : humanityUpgrades;
}

}  // namespace plague