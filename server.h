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
    QHostAddress m_address;
    unsigned short m_port;

    QUrl m_url;
    QNetworkAccessManager *m_network_manager;

private slots:
    void heartbeat_error(QNetworkReply::NetworkError error);
    void heartbeat_ok(QNetworkReply *reply);
    void heartbeat_response(void);
public:
    Server(const QHostAddress &address, const unsigned short port, const QString &station_id);
    ~Server(void);

    void set_url(const QHostAddress &address, const unsigned short port, const QString &station_id);
    QHostAddress address(void) const;
    unsigned short port(void) const;

public slots:
    void send_heartbeat(const QJsonObject &heartbeat) const;
};

#endif // SERVER_H
