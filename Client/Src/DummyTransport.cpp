#include "DummyTransport.hpp"

//ВРЕМЕННАЯ ЗАГЛУШКА для сетевого слоя. Она делают вид, что клиент
//подключается к серверу, но реально никуда не подключаются.
namespace plague {

bool DummyTransport::connectToServer(const char*, int) {
    connected_ = true;
    return true;
}

void DummyTransport::disconnect() {
    connected_ = false;
}

bool DummyTransport::isConnected() const {
    return connected_;
}

bool DummyTransport::send(const ClientPackage&) {
    return connected_;
}

}
