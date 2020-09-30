#include <QHostAddress>
#include <QNetworkReply>

#ifndef SERVER_H
#define SERVER_H


class Server {
private:
    QHostAddress address;
    unsigned short port;

    QUrl url;
public:
    Server(const QHostAddress& address, const unsigned short port);

    void set_url(const QHostAddress& address, const unsigned short port, const QString& station_id);

    void send_heartbeat(void) const;
};

#endif // SERVER_H
