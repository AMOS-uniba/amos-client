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

class QServer: public QAmosWidget {
    Q_OBJECT
private:
    Ui::QServer * ui;
    QNetworkAccessManager * m_network_manager;
    mutable QDateTime m_last_heartbeat;

    QHostAddress m_address;
    unsigned short m_port;
    QString m_station_id;

    QUrl m_url_heartbeat;
    QUrl m_url_sighting;

    void connect_slots(void) override;
    void load_defaults(void) override;
    void load_settings_inner(const QSettings * const settings) override;
    void save_settings_inner(QSettings * settings) const override;
    void apply_changes_inner(void) override;
    void discard_changes_inner(void) override;

    bool is_id_changed(void) const;
    bool is_address_changed(void) const;

private slots:
    void set_address(const QString & address, const unsigned short port);
    void set_station_id(const QString & station_id);

    void heartbeat_error(QNetworkReply::NetworkError error);
    void heartbeat_ok(QNetworkReply * reply);
    void refresh_urls(void);

    void send_sighting(const Sighting & sighting) const;

public:
    explicit QServer(QWidget * parent = nullptr);
    ~QServer();

    bool is_changed(void) const override;

    inline const QHostAddress & address(void) const { return this->m_address; }
    inline const unsigned short & port(void) const { return this->m_port; }
    inline const QString & station_id(void) const { return this->m_station_id; }

public slots:
    void initialize(void) override;

    void button_send_heartbeat(void);
    void display_countdown(void);

    void send_heartbeat(const QJsonObject & heartbeat) const;
    void send_sightings(QVector<Sighting> sightings) const;

signals:
    void request_heartbeat(void) const;
    void heartbeat_sent(void) const;
};

#endif // QSERVER_H
