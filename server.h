#include <QObject>
#include <QHostAddress>
#include <QNetworkReply>
#include <QNetworkAccessManager>

#include "forward.h"

#ifndef SERVER_H
#define SERVER_H


class Server: public QObject {
    Q_OBJECT
private:
    QHostAddress address;
    unsigned short port;

    QUrl url;
    QNetworkAccessManager *network_manager;

private slots:
    void heartbeat_error(QNetworkReply::NetworkError error);
    void heartbeat_ok(QNetworkReply *reply);
    void heartbeat_response(void);
public:
    Server(const QHostAddress &_address, const unsigned short _port, const QString &station_id);
    ~Server(void);

    void set_url(const QHostAddress &address, const unsigned short port, const QString &station_id);
    QHostAddress get_address(void) const;
    unsigned short get_port(void) const;

public slots:
    void send_heartbeat(const QJsonObject &heartbeat) const;
};

#endif // SERVER_H
