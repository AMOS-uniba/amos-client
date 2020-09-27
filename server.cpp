#include "server.h"

Server::Server(const QHostAddress& _address, const unsigned short _port):
    address(_address),
    port(_port) {
}
