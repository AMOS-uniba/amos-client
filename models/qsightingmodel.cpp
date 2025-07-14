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
                    case Sighting::Status::UnknownStation:  [[fallthrough]];
                    case Sighting::Status::Timeout:         [[fallthrough]];
                    case Sighting::Status::UnknownError:    [[fallthrough]];
                    case Sighting::Status::RemoteHostClosed:
                        return QColor(255, 0, 0);
                    case Sighting::Status::Stored:
                        return QColor(0, 192, 0);
                    case Sighting::Status::Discarded:
                        return QColor(160, 0, 0);
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
            case Property::DeferredFor:     return "remaining";
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
        logger.debug(Concern::Sightings, QString("Sighting '%1' already in model, ignoring").arg(sighting.prefix()));
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

void QSightingModel::mark_sent(const QString & sighting_id) {
    auto & sighting = this->sightings()[sighting_id];
    this->set_status(sighting, Sighting::Status::Sent);
}

void QSightingModel::store_sighting(const QString & sighting_id) {
    auto & sighting = this->sightings()[sighting_id];
    this->set_status(sighting, Sighting::Status::Accepted);
    emit this->sighting_accepted(sighting);
}

void QSightingModel::discard_sighting(const QString & sighting_id) {
    auto & sighting = this->sightings()[sighting_id];
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

void QSightingModel::clear(void) {
    this->m_sightings.clear();
    this->beginResetModel();
    this->endResetModel();
}
