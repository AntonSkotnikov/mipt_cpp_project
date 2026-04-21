#pragma once

#include "UIBase.hpp"
#include "UIRequest.hpp"
#include <cstddef>
#include <string_view>

namespace plague::ui {

enum MapResolution : unsigned char {
    Low = 0,
    Medium,
    High
};

enum SettingsItemId  : unsigned char {
    MapResolution = 0,
    Back
};

struct SettingsItem {
    SettingsItemId id;
    int left;
    int right;
    int up;
    int down;

    std::string_view   textOfSetting;
    const std::string_view  * possibleValues;
    bool               isEditable     = false;
    size_t minValue;
    size_t maxValue;

    request::UIRequest action;
};

struct SettingsState {
    size_t selectedItem   = MapResolution;
    size_t settings[Back] = {1};
};

class SettingsScreen final : public Screen {
public:
    SettingsScreen() = default;
    ~SettingsScreen() override = default;

    void draw() const override;
    plague::request::UIRequest handleInput(int key) override;

private:
    SettingsState state_{};
    size_t dump_;
    bool isChanging_;

    void moveUp();
    void moveDown();
    void moveLeft();
    void moveRight();
    void endChangeWithoutSave();
    void endChangeWithSave();
    request::UIRequest activateCurrentItem();

    static constexpr std::string_view mapResolutions[] = {"Low", "Medium", "High"};

    static constexpr SettingsItem kSettingsItems[] = {
        {SettingsItemId::MapResolution, -1, -1, -1,  1, 
         "Map Resolution", mapResolutions, false, 0, 2,
         request::None{}},
        {SettingsItemId::Back,          -1, -1, 0, -1, 
         "Exit", {}, false, 0, 0,
          request::Settings::Back}
    };
};

}