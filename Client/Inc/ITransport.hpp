#pragma once

#include "ClientPackage.hpp"

namespace plague {

class ITransport {
public:
    virtual ~ITransport() = default;

    virtual bool connectToServer(const char* host, int port) = 0;
    virtual void disconnect() = 0;
    virtual bool isConnected() const = 0;
    virtual bool send(const ClientPackage& package) = 0;
};

}
