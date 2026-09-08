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
    constexpr static float DeferTime = 60;                  // Time in seconds: how long to defer a Sighting
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
    /* Look a sighting up without creating one. QMap::operator[] default-constructs a value for an
     * unknown key, and a reply naming a sighting the model no longer holds -- one cleared from the
     * buffer while its upload was in flight -- used to conjure an empty entry with no prefix, no
     * files and an invalid timestamp. Being neither deferred nor finished, it was then offered to
     * the server every minute for as long as the client ran.
     */
    QMap<QString, Sighting>::iterator find_sighting(const QString & sighting_id);

private slots:
    void update_timers(void);
    void set_status(Sighting & sighting, Sighting::Status status);
public slots:
    void send_sightings(void);
    void force_send_sightings(void);
    void insert_sighting(const Sighting & sighting);        // found by camera
    void mark_stored(Sighting & sighting);                  // stored by camera
    void mark_quarantined(Sighting & sighting);             // quarantined by camera

    void mark_sent(const QString & sighting_id);            // sent by server, but no response so far
    void store_sighting(const QString & sighting_id, bool metadata_stored);  // accepted by server
    void quarantine_sighting(const QString & sighting_id);     // rejected by server
    void defer_sighting(const QString & sighting_id, QNetworkReply::NetworkError error);

    // Remove the sightings that have been stored, leaving anything with work left to do. The only
    // thing that ever takes a row out of the model, and only on an explicit button press.
    void remove_stored(void);
signals:
    void sighting_to_send(const Sighting & sighting);
    void sighting_stored(Sighting & sighting);
    void sighting_quarantined(Sighting & sighting);
    void sighting_accepted(Sighting & sighting);
    void sighting_rejected(Sighting & sighting);
    void sighting_deferred(Sighting & sighting);
};

#endif // QSIGHTINGMODEL_H
