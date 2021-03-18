#ifndef QSERVER_H
#define QSERVER_H

#include <QGroupBox>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "widgets/qconfigurable.h"
#include "utils/sighting.h"
#include "utils/exception.h"
#include "logging/eventlogger.h"

namespace Ui {
    class QServer;
}

class QServer: public QGroupBox {
    Q_OBJECT
private:
    Ui::QServer *ui;
    Station *m_station;

    QHostAddress m_address;
    unsigned short m_port;
    QString m_station_id;

    QUrl m_url_heartbeat;
    QUrl m_url_sighting;
    QNetworkAccessManager *m_network_manager;

    bool is_changed(void);

private slots:
    void load_settings(const QSettings * const settings);
    void save_settings(void);

    void heartbeat_error(QNetworkReply::NetworkError error);
    void heartbeat_ok(QNetworkReply * reply);
    void refresh_urls(void);

    void handle_settings_changed(void);

public:
    explicit QServer(QWidget *parent = nullptr);
    ~QServer();

    const QHostAddress &address(void) const;
    const unsigned short &port(void) const;
    const QString &station_id(void) const;

public slots:
    void initialize(Station * const station);

    void set_address(const QString &address, const unsigned short port);
    void set_station_id(const QString &station_id);

    void apply_settings(void);
    void discard_settings(void);

    void button_send_heartbeat(void);

    void send_heartbeat(const QJsonObject &heartbeat) const;
    void send_sighting(const Sighting &sighting) const;

signals:
    void settings_changed(void) const;

    void heartbeat_sent(void) const;
};

#endif // QSERVER_H
