#pragma once

#include "Client_ServerAPI.hpp"

#include <string>

namespace plague {

struct ClientPackage {
    ClientCommand command{};
    std::string payload{};
};

}
