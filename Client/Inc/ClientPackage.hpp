#pragma once

#include "Client_ServerAPI.hpp"

#include <string>

namespace plague {

// Минимальный пакет, который клиент отправляет в wire-протокол.
struct ClientPackage {
    ClientCommand command{};
    std::string payload{};
};

}
