#ifndef QSIGHTINGMODEL_H
#define QSIGHTINGMODEL_H

#include <QAbstractTableModel>
#include <QObject>

QT_FORWARD_DECLARE_CLASS(QSightingBuffer);

class QSightingModel: public QAbstractTableModel {
    Q_OBJECT
private:
    typedef enum {
        ID = 0,
        Status = 1,
        DeferredUntil = 2,
        DeferredFor = 3,
    } Property;
    const QSightingBuffer * m_buffer;
public:
    QSightingModel(const QSightingBuffer * buffer, QObject * parent = nullptr);

    int rowCount(const QModelIndex & parent = QModelIndex()) const override;
    int columnCount(const QModelIndex & parent = QModelIndex()) const override;
    QVariant data(const QModelIndex & index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

public slots:
    void update_data();
};

#endif // QSIGHTINGMODEL_H
