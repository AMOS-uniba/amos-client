#include <QHostAddress>
#include <QNetworkReply>

#ifndef SERVER_H
#define SERVER_H


class Server {
private:
    QHostAddress address;
    unsigned short port;
public:
    Server(const QHostAddress& address, const unsigned short port);

    QNetworkReply send_heartbeat(void) const;
};

#endif // SERVER_H
