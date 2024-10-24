#include "qsightingmodel.h"

#include <widgets/qcamera.h>

QSightingModel::QSightingModel(const QCamera * camera, QObject * parent):
    QAbstractTableModel(parent),
    m_camera(camera)
{
}

int QSightingModel::rowCount(const QModelIndex & index) const {
    Q_UNUSED(index);
    qDebug() << this->m_camera->deferred_sightings().count() + 1;
    return this->m_camera->deferred_sightings().count() + 1;
}

int QSightingModel::columnCount(const QModelIndex & index) const {
    Q_UNUSED(index);
    return 4;
}

QVariant QSightingModel::data(const QModelIndex & index, int role) const {
    if (!index.isValid() || (index.row() >= this->m_camera->deferred_sightings().count())) {
        return QVariant();
    }

    auto sighting = std::next(this->m_camera->deferred_sightings().constBegin(), index.row());
    switch (role) {
        case Qt::DisplayRole: {
            switch (index.column()) {
                case Property::ID:              return sighting.key();
                case Property::Status:			return sighting.value();
                case Property::DeferredUntil:	return sighting.value();
                case Property::DeferredFor:	    {
                    int ms = (sighting.value() - QDateTime::currentDateTime()).count() / 1000;
                    return ms >= 0 ? QString("%1").arg(ms) : "0";
                }
                default:                        return QVariant();
            }
            break;
        }
        case Qt::TextAlignmentRole: {
            return int(Qt::AlignRight | Qt::AlignVCenter);
            break;
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
            case Property::DeferredUntil:	return "deferred until";
            case Property::Status:			return "status";
            case Property::DeferredFor:     return "remaining";
            default:						return QVariant();
        }
    } else {
        return QString("%1").arg(section);
    }
}

void QSightingModel::update_data(void) {
    qDebug() << "Updating data";
    emit this->dataChanged(this->index(0, 0), this->index(this->columnCount() - 1, this->rowCount() - 1), {Qt::DisplayRole});
}
