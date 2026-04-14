#pragma once
//ВРЕМЕННАЯ ЗАГЛУШКА для сетевого слоя. Она делают вид, что клиент
//подключается к серверу, но реально никуда не подключаются.
#include "ITransport.hpp"

namespace plague {

class DummyTransport final : public ITransport {
public:
    bool connectToServer(const char* host, int port) override;
    void disconnect() override;
    bool isConnected() const override;
    bool send(const ClientPackage& package) override;

private:
    bool connected_ = false;
};

}
