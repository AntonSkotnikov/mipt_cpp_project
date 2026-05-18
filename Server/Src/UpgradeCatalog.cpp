#include "UpgradeCatalog.hpp"

namespace plague {

namespace {

const std::vector<UpgradeDefinition> pathogenUpgrades = {
    {"air_1", UpgradeCategory::Transmission, "Air I", 3, "Improves airborne transmission.\n\nEffects are not implemented yet.", {}},
    {"water_1", UpgradeCategory::Transmission, "Water I", 4, "Improves water transmission.\n\nEffects are not implemented yet.", {}},
    {"air_2", UpgradeCategory::Transmission, "Air II", 7, "A stronger airborne transmission upgrade.\n\nRequires Air I.", {"air_1"}},
    {"cure_delay", UpgradeCategory::Clinic, "Cure Delay", 5, "Slows research progress.\n\nEffects are not implemented yet.", {}},
    {"clinic_overload", UpgradeCategory::Clinic, "Clinic Overload", 8, "Pressures medical systems.\n\nRequires Cure Delay.", {"cure_delay"}},
    {"research_noise", UpgradeCategory::Clinic, "Research Noise", 6, "Makes cure data less reliable.\n\nEffects are not implemented yet.", {}},
    {"cold_resistance", UpgradeCategory::Abilities, "Cold Resistance", 4, "Improves survival in cold regions.\n\nEffects are not implemented yet.", {}},
    {"heat_resistance", UpgradeCategory::Abilities, "Heat Resistance", 4, "Improves survival in hot regions.\n\nEffects are not implemented yet.", {}},
    {"drug_resistance", UpgradeCategory::Abilities, "Drug Resistance", 9, "Improves survival in wealthy regions.\n\nRequires Cold Resistance and Heat Resistance.", {"cold_resistance", "heat_resistance"}}
};

const std::vector<UpgradeDefinition> humanityUpgrades = {
    {"vaccine_1", UpgradeCategory::Clinic, "Vaccine I", 10, "Improves cure research.\n\nEffects are not implemented yet.", {}},
    {"quarantine_1", UpgradeCategory::Transmission, "Quarantine I", 15, "Reduces disease spread through travel restrictions.\n\nEffects are not implemented yet.", {}}
};

}  // namespace

const std::vector<UpgradeDefinition>& availableUpgradesFor(PlayerRole role) {
    return role == PlayerRole::Pathogen ? pathogenUpgrades : humanityUpgrades;
}

}  // namespace plague
