#pragma once

#include <string>

namespace plague {

enum class ClientCommand {
    Connect,
    Disconnect,
    ChooseHumanity,
    ChoosePathogen,
    Ping
};

struct ClientPackage {
    ClientCommand command{};
    std::string payload{};
};

}
