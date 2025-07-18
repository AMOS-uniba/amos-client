#ifndef QSERVER_H
#define QSERVER_H

#include <QGroupBox>
#include <QHostAddress>
#include <QNetworkAccessManager>
#include <QNetworkReply>

#include "widgets/qconfigurable.h"
#include "utils/sighting.h"

namespace Ui {
    class QServer;
}


class QServer: public QAmosWidget {
    Q_OBJECT
private:
    Ui::QServer * ui;
    QNetworkAccessManager * m_heartbeat_manager;
    QNetworkAccessManager * m_sighting_manager;

    QTimer * m_timer_heartbeat;
    mutable QDateTime m_last_heartbeat;

    QHostAddress m_address;
    unsigned short m_port;
    QString m_station_id;
    int m_heartbeat_interval;

    QUrl m_url_heartbeat;
    QUrl m_url_sighting;

    void connect_slots(void) override;
    void load_defaults(void) override;
    void load_settings_inner(void) override;
    void save_settings_inner(void) const override;
    void apply_changes_inner(void) override;
    void discard_changes_inner(void) override;

    bool is_id_changed(void) const;
    bool is_address_changed(void) const;
    bool is_interval_changed(void) const;

private slots:
    void set_address(const QString & address, const unsigned short port);
    void set_station_id(const QString & station_id);
    void set_heartbeat_interval(unsigned int interval);

    void heartbeat_error(QNetworkReply::NetworkError error);
    void heartbeat_finished(QNetworkReply * reply);
    void refresh_urls(void);

    void on_le_station_id_textChanged(const QString & text);
    void on_sb_interval_valueChanged(int value);
    void on_le_ip_textChanged(const QString & text);
    void on_sb_port_valueChanged(int value);

public:
    explicit QServer(QWidget * parent = nullptr);
    ~QServer();

    bool is_changed(void) const override;

    inline const QHostAddress & address(void) const { return this->m_address; }
    inline const unsigned short & port(void) const { return this->m_port; }
    inline const QString & station_id(void) const { return this->m_station_id; }
    inline const int & heartbeat_interval(void) const { return this->m_heartbeat_interval; }

    inline const QTimer * timer_heartbeat(void) const { return this->m_timer_heartbeat; }

public slots:
    void initialize(QSettings * settings) override;

    void button_send_heartbeat(void);
    void display_countdown(void);

    void send_heartbeat(const QJsonObject & heartbeat) const;
    void send_sighting(const Sighting & sighting) const;

    void sighting_received(QNetworkReply * reply);

signals:
    void request_heartbeat(void);
    void heartbeat_created(void);

    void sighting_sent(const QString & sighting_id) const;
    // Sighting was accepted, store it
    void sighting_accepted(const QString & sighting_id);
    // Sighting was rejected, delete it
    void sighting_conflict(const QString & sighting_id);
    // Sighting could not reach server, defer and try again later
    void sighting_error(const QString & sighting_id, QNetworkReply::NetworkError error);
};

#endif // QSERVER_H
