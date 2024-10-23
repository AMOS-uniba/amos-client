#include "qsightingmodel.h"

#include <widgets/qcamera.h>

QSightingModel::QSightingModel(const QCamera * camera, QObject * parent):
    QAbstractTableModel(parent),
    m_camera(camera)
{}

int QSightingModel::rowCount(const QModelIndex & index) const {
    Q_UNUSED(index);
    return this->m_camera->deferred_sightings().count();
}

int QSightingModel::columnCount(const QModelIndex & index) const {
    Q_UNUSED(index);
    return 3;
}

QString QSightingModel::title(int role) const {
    switch (role) {
        case Property::ID:              return "ID";
        case Property::DeferredUntil:   return "deferred until";
        case Property::Status:          return "status";
        default:                        return "";
    }
}

QString QSightingModel::value(const QModelIndex & index) const {
    //const Sighting & sighting = this->m_camera->deferred_sightings();
    const auto value = std::next(this->m_camera->deferred_sightings().constBegin(), index.row()).key();
    switch (index.column()) {
        case Property::ID:              return value;
        default:                        return "";
    }
}

QVariant QSightingModel::data(const QModelIndex & index, int role) const {
    if (!index.isValid() || (index.row() >= this->m_camera->deferred_sightings().count())) {
        return QVariant();
    }

    switch (role) {
        case Qt::DisplayRole: {
            return QString();
            break;
        }
        default: {
            return QVariant();
        }
    }
}


QVariant QSightingModel::headerData(int section, Qt::Orientation orientation, int role) const {

}
