#ifndef QSIGHTINGMODEL_H
#define QSIGHTINGMODEL_H

#include <QAbstractTableModel>
#include <QObject>
#include <QNetworkReply>
#include <QTimer>

#include "utils/sighting.h"

QT_FORWARD_DECLARE_CLASS(QSightingBuffer);

class QSightingModel: public QAbstractTableModel {
    Q_OBJECT
private:
    constexpr static float DeferTime = 5;                   // Time in seconds: how long to defer a Sighting
    constexpr static int DeferRefreshInterval = 100;        // Time in ms: how often to refresh the view
    constexpr static int SendInterval = 5000;               // Time in ms: how often to try to send sightings

    typedef enum {
        ID = 0,
        Spectral,
        Timestamp,
        Size,
        DeferredFor,
        Status,
    } Property;

    QMap<QString, Sighting> m_sightings;

    QTimer * m_display_timer;
    QTimer * m_send_timer;

    virtual bool insertRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
    virtual bool removeRows(int row, int count, const QModelIndex & parent = QModelIndex()) override;
public:
    QSightingModel(QObject * parent = nullptr);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    inline QMap<QString, Sighting> & sightings(void) { return this->m_sightings; }
    inline const QMap<QString, Sighting> & sightings(void) const { return this->m_sightings; }
private slots:
    void update_timers(void);
    void set_status(Sighting & sighting, Sighting::Status status);
public slots:
    void send_sightings(void);
    void force_send_sightings(void);
    void insert_sighting(const Sighting & sighting);        // found by camera
    void mark_stored(Sighting & sighting);                  // stored by camera
    void mark_discarded(Sighting & sighting);               // discarded by camera

    void mark_sent(const QString & sighting_id);            // sent by server, but no response so far
    void store_sighting(const QString & sighting_id);       // accepted by server
    void discard_sighting(const QString & sighting_id);     // rejected by server
    void defer_sighting(const QString & sighting_id, QNetworkReply::NetworkError error);

    void reload(void);
signals:
    void sighting_to_send(const Sighting & sighting);
    void sighting_deleted(Sighting & sighting);
    void sighting_stored(Sighting & sighting);
    void sighting_accepted(Sighting & sighting);
    void sighting_rejected(Sighting & sighting);
    void sighting_deferred(Sighting & sighting);
};

#endif // QSIGHTINGMODEL_H
