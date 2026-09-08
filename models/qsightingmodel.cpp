#include "qsightingmodel.h"
#include "widgets/qsightingbuffer.h"
#include "logging/eventlogger.h"

extern EventLogger logger;


QSightingModel::QSightingModel(QObject * parent):
    QAbstractTableModel(parent)
{
    this->m_display_timer = new QTimer();
    this->m_display_timer->setInterval(QSightingModel::DeferRefreshInterval);
    this->connect(this->m_display_timer, &QTimer::timeout, this, &QSightingModel::update_timers);
    this->m_display_timer->start();

    this->m_send_timer = new QTimer();
    this->m_send_timer->setInterval(QSightingModel::SendInterval);
    this->connect(this->m_send_timer, &QTimer::timeout, this, &QSightingModel::send_sightings);
    this->m_send_timer->start();
}

int QSightingModel::rowCount(const QModelIndex & index) const {
    Q_UNUSED(index);
    return this->sightings().count();
}

int QSightingModel::columnCount(const QModelIndex & index) const {
    Q_UNUSED(index);
    return 6;
}

QVariant QSightingModel::data(const QModelIndex & index, int role) const {
    if (!index.isValid() || (index.row() >= this->sightings().count())) {
        return QVariant();
    }

    auto item = std::next(this->sightings().constBegin(), index.row());
    auto && sighting = item.value();
    switch (role) {
        case Qt::DisplayRole: {
            switch (index.column()) {
                case Property::ID:
                    return item.key();
                case Property::Spectral:
                    return sighting.spectral_string();
                case Property::Timestamp:
                    return sighting.timestamp().toString("yyyy-MM-dd hh:mm:ss");
                case Property::Size:
                    return QString("%1 KiB").arg(sighting.avi_size() >> 10);
                case Property::Status:
                    return sighting.status_string();
                case Property::DeferredFor:	    {
                    double time = sighting.deferred_for();
                    if (time > 0) {
                        return QString("%1 s").arg(time, 0, 'f', 1);
                    } else {
                        return QString("— s");
                    }
                }
                default: {
                    return QVariant();
                }
            }
            break;
        }
        case Qt::TextAlignmentRole: {
            switch (index.column()) {
                case Property::ID:
                    [[fallthrough]];
                case Property::DeferredFor:
                    [[fallthrough]];
                case Property::Size:
                    return int(Qt::AlignRight | Qt::AlignVCenter);
                default:
                    return int(Qt::AlignCenter | Qt::AlignVCenter);
            }
            break;
        }
        case Qt::ForegroundRole: {
            if (index.column() == Property::Status) {
                switch (sighting.status()) {
                    case Sighting::Status::Unprocessed:
                        return QColor(0, 0, 0);
                    case Sighting::Status::Sent:
                        return QColor(160, 160, 160);
                    case Sighting::Status::Accepted:
                        return QColor(0, 224, 0);
                    case Sighting::Status::Duplicate:       [[fallthrough]];
                    case Sighting::Status::Rejected:        [[fallthrough]];
                    case Sighting::Status::Timeout:         [[fallthrough]];
                    case Sighting::Status::UnknownError:    [[fallthrough]];
                    case Sighting::Status::RemoteHostClosed:
                        return QColor(255, 0, 0);
                    case Sighting::Status::Stored:
                        return QColor(0, 192, 0);
                    case Sighting::Status::Quarantined:
                        return QColor(192, 0, 128);
                }
            } else {
                return QVariant();
            }
            break;
        }
        default: {
            return QVariant();
        }
    }
    return QVariant();
}

QVariant QSightingModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal) {
        switch (section) {
            case Property::ID: 				return "ID";
            case Property::Spectral:        return "kind";
            case Property::Timestamp:       return "timestamp";
            case Property::Size:            return "AVI size";
            case Property::Status:			return "status";
            case Property::DeferredFor:     return "try again in";
            default:						return QVariant();
        }
    } else {
        return QString("%1").arg(section);
    }
}

void QSightingModel::send_sightings(void) {
    for (auto && sighting: this->sightings()) {
        if ((!sighting.is_deferred()) && (!sighting.is_finished())) {
            emit this->sighting_to_send(sighting);
            sighting.defer(QSightingModel::DeferTime);
        }
    }
    emit this->dataChanged(this->index(0, Property::Status), this->index(this->rowCount() - 1, Property::Status));
}

void QSightingModel::force_send_sightings(void) {
    for (auto && sighting: this->sightings()) {
        sighting.undefer();
    }
    this->send_sightings();
}

void QSightingModel::update_timers(void) {
    emit this->dataChanged(this->index(0, Property::DeferredFor), this->index(this->rowCount() - 1, Property::DeferredFor));
}

void QSightingModel::insert_sighting(const Sighting & sighting) {
    if (this->m_sightings.contains(sighting.prefix())) {
        /* A Sighting is a snapshot of the files the scanner decided it owned. While it has never
         * been sent, the newer snapshot is the better one: it may have picked up a part that arrived
         * late, and it may equally have lost one, since a file is handed to the longest metadata
         * prefix matching it and a sibling's metadata appearing shifts that verdict. Once the
         * sighting has been sent, the snapshot is what the server was told and what move() will look
         * for, so it is left alone.
         */
        Sighting & known = this->m_sightings[sighting.prefix()];
        if (known.status() == Sighting::Status::Unprocessed) {
            logger.debug(Concern::Sightings, QString("Refreshing Sighting '%1'").arg(sighting.prefix()));
            known = sighting;
            const int row = std::distance(this->sightings().constBegin(),
                                          this->sightings().constFind(sighting.prefix()));
            emit this->dataChanged(this->index(row, 0), this->index(row, this->columnCount() - 1));
        } else {
            logger.debug(Concern::Sightings, QString("Sighting '%1' already in model, ignoring").arg(sighting.prefix()));
        }
    } else {
        logger.debug(Concern::Sightings, QString("Adding Sighting '%1").arg(sighting.prefix()));
        this->m_sightings.insert(sighting.prefix(), sighting);
        this->insertRow(this->rowCount());
    }
}

bool QSightingModel::insertRows(int row, int count, const QModelIndex & index) {
    this->beginInsertRows(index, row, row + count - 1);
    this->endInsertRows();
    return true;
}

/** Remove count rows starting at row.
 *
 *  Nothing calls this yet -- no sighting is ever taken out of the model, which is why it grows for
 *  as long as the client runs -- but it was wrong in three ways and would have misbehaved the first
 *  time it was used. It sought to `index.row()` rather than to `row`, and `index` is the *parent*
 *  index, which for a table model is always invalid: that made it std::next(constBegin(), -1). It
 *  removed a key through the very iterator it then advanced, and QMap::remove invalidates
 *  iterators. And it checked neither bound, so a bad range walked off the end of the map.
 *
 *  Hence collecting the keys before erasing anything: the walk and the removal no longer overlap.
**/
bool QSightingModel::removeRows(int row, int count, const QModelIndex & parent) {
    if ((row < 0) || (count <= 0) || (row + count > this->rowCount())) {
        logger.debug_error(Concern::Sightings,
                           QString("Refusing to remove %1 row(s) from row %2, the model holds %3")
                               .arg(count).arg(row).arg(this->rowCount()));
        return false;
    }

    QStringList keys;
    keys.reserve(count);
    auto item = std::next(this->sightings().constBegin(), row);
    for (int i = 0; i < count; ++i, ++item) {
        keys.append(item.key());
    }

    this->beginRemoveRows(parent, row, row + count - 1);
    for (const QString & key: keys) {
        this->m_sightings.remove(key);
    }
    this->endRemoveRows();
    return true;
}

void QSightingModel::set_status(Sighting & sighting, Sighting::Status status) {
    sighting.set_status(status);
    emit this->dataChanged(this->index(0, Property::DeferredFor), this->index(this->rowCount() - 1, Property::Status));
}

void QSightingModel::mark_stored(Sighting & sighting) {
    this->set_status(sighting, Sighting::Status::Stored);
}

void QSightingModel::mark_quarantined(Sighting & sighting) {
    this->set_status(sighting, Sighting::Status::Quarantined);
}

/** Find the sighting a server answer concerns, if it is still one this model may act on.
 *
 *  A finished sighting is not. Its files have been moved to permanent storage, so there is nothing
 *  left to do with it and nothing that should be done to it. A second, late answer -- and the
 *  client re-sends every DeferTime while it waits, so any answer slower than that produces two --
 *  used to put a stored sighting back to accepted. The camera then refused it for sitting outside
 *  the scan directory, which left it accepted: neither finished nor deferred, and so picked up
 *  again by every send pass from then on. What went up each time was read from paths that no
 *  longer exist, which is to say nothing at all.
**/
QMap<QString, Sighting>::iterator QSightingModel::find_sighting(const QString & sighting_id) {
    auto sighting = this->m_sightings.find(sighting_id);
    if (sighting == this->m_sightings.end()) {
        logger.debug_error(Concern::Sightings,
                           QString("Sighting '%1' is not in the model, ignoring the server's answer")
                               .arg(sighting_id));
        return sighting;
    }

    if (sighting->is_finished()) {
        logger.debug(Concern::Sightings,
                     QString("Sighting '%1' is already %2, ignoring the server's answer")
                         .arg(sighting_id, sighting->status_string()));
        return this->m_sightings.end();
    }

    return sighting;
}

void QSightingModel::mark_sent(const QString & sighting_id) {
    auto found = this->find_sighting(sighting_id);
    if (found == this->sightings().end()) {
        return;
    }
    Sighting & sighting = *found;
    this->set_status(sighting, Sighting::Status::Sent);
}

void QSightingModel::store_sighting(const QString & sighting_id, bool metadata_stored) {
    auto found = this->find_sighting(sighting_id);
    if (found == this->sightings().end()) {
        return;
    }
    Sighting & sighting = *found;

    /* The server keeps no metadata when no xml or yaml part reached it. For a sighting that had none
     * to send that is the expected answer: the reduction has not run yet, and its metadata will
     * follow as a delivery of its own which the server merges onto the same row. For one that did
     * send metadata it means the part did not arrive under the name the server looks for, which is a
     * fault on this side -- so do not accept it, or the files are moved to permanent storage while
     * the row on the server stays empty and nothing is ever retried.
     */
    if (sighting.has_metadata() && !metadata_stored) {
        logger.error(Concern::Sightings,
                     QString("Sighting '%1' was accepted but its metadata was not stored, will try again")
                         .arg(sighting_id));
        this->set_status(sighting, Sighting::Status::Rejected);
        sighting.defer(QSightingModel::DeferTime);
        emit this->sighting_deferred(sighting);
        return;
    }

    this->set_status(sighting, Sighting::Status::Accepted);
    emit this->sighting_accepted(sighting);
}

void QSightingModel::quarantine_sighting(const QString & sighting_id) {
    auto found = this->find_sighting(sighting_id);
    if (found == this->sightings().end()) {
        return;
    }
    Sighting & sighting = *found;
    this->set_status(sighting, Sighting::Status::Rejected);
    emit this->sighting_rejected(sighting);
}

void QSightingModel::defer_sighting(const QString & sighting_id, QNetworkReply::NetworkError error) {
    auto found = this->find_sighting(sighting_id);
    if (found == this->sightings().end()) {
        return;
    }
    Sighting & sighting = *found;
    switch (error) {
        // HTTP 400: refused, but retried anyway -- see QServer::sighting_received
        case QNetworkReply::ProtocolInvalidOperationError:
            [[fallthrough]];
        case QNetworkReply::UnknownContentError: {
            this->set_status(sighting, Sighting::Status::Rejected);
            break;
        }
        case QNetworkReply::RemoteHostClosedError: {
            this->set_status(sighting, Sighting::Status::RemoteHostClosed);
            break;
        }
        case QNetworkReply::TimeoutError: {
            this->set_status(sighting, Sighting::Status::Timeout);
            break;
        }
        default: {
            this->set_status(sighting, Sighting::Status::UnknownError);
            break;
        }
    }
    sighting.defer(QSightingModel::DeferTime);
    emit this->sighting_deferred(sighting);
}

/** Drop the sightings that are done with. The Clear button is the only thing that calls this, and
 *  the only thing that ever takes a row out of the model at all -- until it is pressed, the table
 *  is the whole history of the session.
 *
 *  Only the stored ones go. A sighting still unprocessed, in flight, or deferred after a refusal
 *  has work left to do, and dropping it would take it out of the send path while its files sit in
 *  the scan directory waiting to be delivered. Quarantined ones stay as well: they are the record
 *  that something went wrong with a capture, and nothing else on screen says so.
 *
 *  This replaces a clear() that emptied the model wholesale -- pending deliveries included -- and
 *  did it before calling beginResetModel() rather than after, which is the wrong way round.
**/
void QSightingModel::remove_stored(void) {
    int removed = 0;

    /* Backwards, so that removing a row cannot shift the ones not yet looked at. One row at a time
     * because the stored ones are not necessarily adjacent; this runs on a button press, not in
     * any loop, so the repeated lookup does not matter.
     */
    for (int row = this->rowCount() - 1; row >= 0; --row) {
        const auto item = std::next(this->sightings().constBegin(), row);
        if (item->status() == Sighting::Status::Stored) {
            this->removeRows(row, 1);
            ++removed;
        }
    }

    logger.info(Concern::Sightings,
                QString("Removed %1 stored sighting(s) from the buffer, %2 still listed")
                    .arg(removed).arg(this->rowCount()));
}
