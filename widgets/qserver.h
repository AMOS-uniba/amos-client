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

class QServer: public QConfigurable {
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

    bool is_changed(void) override;

private slots:
    void load_settings_inner(const QSettings * const settings) override;
    void save_settings(void) override;

    void heartbeat_error(QNetworkReply::NetworkError error);
    void heartbeat_ok(QNetworkReply * reply);
    void refresh_urls(void);

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

    void apply_settings_inner(void) override;
    void discard_settings(void) override;

    void button_send_heartbeat(void);

    void send_heartbeat(const QJsonObject &heartbeat) const;
    void send_sighting(const Sighting &sighting) const;

signals:
    void settings_changed(void) const;

    void heartbeat_sent(void) const;
};

#endif // QSERVER_H
