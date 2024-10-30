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
    return 7;
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
                    return QString("%1 kiB").arg(sighting.avi_size() >> 10);
                case Property::Status:
                    return sighting.status_string();
                case Property::DeferredUntil:	{
                    if (sighting.deferred_until().isValid()) {
                        return sighting.deferred_until().toString("hh:mm:ss");
                    } else {
                        return "not deferred";
                    }
                }
                case Property::DeferredFor:	    {
                    double time = sighting.deferred_for();
                    if (time > 0) {
                        return QString("%1 s").arg(time, 0, 'f', 1);
                    } else {
                        return QVariant();
                    }
                }
                // case Property::Address: {
                //     return QString("0x%1").arg(qulonglong(&sighting), 16, 16);
                // }
                default:
                    return QVariant();
            }
            break;
        }
        case Qt::TextAlignmentRole: {
            switch (index.column()) {
                case Property::ID:          return int(Qt::AlignRight | Qt::AlignVCenter);
                case Property::DeferredFor: return int(Qt::AlignRight | Qt::AlignVCenter);
                default:                    return int(Qt::AlignCenter | Qt::AlignVCenter);
            }
            break;
        }
        case Qt::ForegroundRole: {
            if (index.column() == Property::Status) {
                switch (sighting.status()) {
                    case Sighting::Status::Accepted: return QColor(0, 192, 0);
                    case Sighting::Status::Stored: return QColor(0, 160, 0);
                    case Sighting::Status::Rejected: return QColor(192, 0, 0);
                    case Sighting::Status::Discarded: return QColor(128, 0, 0);
                    case Sighting::Status::UnknownStation: return QColor(255, 0, 0);
                    case Sighting::Status::Duplicate: return QColor(255, 0, 0);
                    case Sighting::Status::Sent: return QColor(160, 160, 160);
                    default: return QVariant();
                }
            } else {
                return QVariant();
            }
        }
        default: {
            return QVariant();
        }
    }
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
            case Property::DeferredUntil:	return "deferred until";
            case Property::DeferredFor:     return "remaining";
            //case Property::Address:         return "address in memory";
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
}

void QSightingModel::update_timers(void) {
    emit this->dataChanged(this->index(0, Property::DeferredFor), this->index(this->rowCount() - 1, Property::DeferredFor));
}

void QSightingModel::insert_sighting(const Sighting & sighting) {
    if (this->m_sightings.contains(sighting.prefix())) {
        logger.debug(Concern::Sightings, QString("Sighting '%1' already in model, ignoring").arg(sighting.prefix()));
    } else {
        logger.debug(Concern::Sightings, QString("Inserting Sighting '%1").arg(sighting.prefix()));
        this->m_sightings.insert(sighting.prefix(), sighting);
        this->insertRow(this->rowCount());
    }
}

bool QSightingModel::insertRows(int row, int count, const QModelIndex & index) {
    this->beginInsertRows(index, row, row + count - 1);
    this->endInsertRows();
    return true;
}

bool QSightingModel::removeRows(int row, int count, const QModelIndex & index) {
    this->beginRemoveRows(index, row, row + count - 1);
    auto item = std::next(this->sightings().constBegin(), index.row());
    for (int i = row; i < row + count; ++i) {
        this->sightings().remove(item.key());
        item = std::next(item);
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

void QSightingModel::mark_discarded(Sighting & sighting) {
    this->set_status(sighting, Sighting::Status::Discarded);
}

void QSightingModel::store_sighting(const QString & sighting_id) {
    auto & sighting = this->sightings()[sighting_id];
    qDebug() << "Storing " << &sighting;
    this->set_status(sighting, Sighting::Status::Accepted);
    emit this->sighting_accepted(sighting);
}

void QSightingModel::discard_sighting(const QString & sighting_id) {
    auto & sighting = this->sightings()[sighting_id];
    qDebug() << "Discarding " << &sighting;
    this->set_status(sighting, Sighting::Status::Rejected);
    emit this->sighting_rejected(sighting);
}

void QSightingModel::defer_sighting(const QString & sighting_id, QNetworkReply::NetworkError error) {
    auto & sighting = this->sightings()[sighting_id];
    switch (error) {
        case QNetworkReply::UnknownContentError: {
            this->set_status(sighting, Sighting::Status::UnknownStation);
            break;
        }
        case QNetworkReply::TimeoutError: {
            this->set_status(sighting, Sighting::Status::Unprocessed);
            break;
        }
        default: {
            break;
        }
    }
    sighting.defer(QSightingModel::DeferTime);
    emit this->sighting_deferred(sighting);
}
