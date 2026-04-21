#pragma once

#include "UIBase.hpp"
#include <string>

namespace plague::ui {

enum class ConnectItemId {
    Address,
    Port,
    Connect,
    Back
};

struct ConnectState {
    ConnectItemId selectedItem = ConnectItemId::Address;

    std::string address        = "";
    std::string port           = "";
};

class ConnectScreen final : public Screen {
public:
    ConnectScreen() = default;
    ~ConnectScreen() override = default;

    void draw() const override;
    plague::request::UIRequest handleInput(int key) override;
private:
    ConnectState state_{};
};

}