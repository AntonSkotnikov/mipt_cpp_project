#pragma once

#include <cstdint>
#include <string>

namespace plague {

using UpgradePointType = std::uint16_t;

enum class PlayerRole {
    Humanity,
    Pathogen
};

enum class GameSituation {
    MainMenu,
    Settings,
    ConnectToServer,
    Exit,
    ChoosingSide,
    Game,
    EndScreen,
};

// Humanity Upgrades

enum class HumanityAbilities {

};

enum class VaccineAbilities {

};

enum class AntiSpreadAbilities {

};

// Pathogen Upgrades

enum class SpreadAbilities {

};

enum class SymphtomsAbilities {

};

enum class PathogenAbilities {

};

//

struct InfoAboutPlayer {
    PlayerRole       role;
    UpgradePointType points;
};

enum class ImportanceOfNews {
    RegularNews,
    ImportantNews,
};

struct News {
    ImportanceOfNews level_;
    std::string text_;

public:
    News(ImportanceOfNews level, std::string text) : level_(level), text_(text) {}
};



}