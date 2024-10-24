#ifndef QSIGHTINGBUFFER_H
#define QSIGHTINGBUFFER_H

#include <QGroupBox>

#include "utils/sighting.h"

QT_FORWARD_DECLARE_CLASS(QSightingModel);

namespace Ui {
    class QSightingBuffer;
}

class QSightingBuffer: public QGroupBox {
    Q_OBJECT
private:
    Ui::QSightingBuffer * ui;
    QMap<QString, Sighting> m_sightings;
    QSightingModel * m_sighting_model;
public:
    explicit QSightingBuffer(QWidget * parent = nullptr);
    ~QSightingBuffer();

    inline const QMap<QString, Sighting> & sightings(void) const { return this->m_sightings; }

    constexpr static float DeferTime = 30;            // Time in seconds: how long to defer an unsent sighting
public slots:
    void insert(const Sighting & sighting);
    void remove(const QString & sighting_id);
    void defer(const QString & sighting_id);
};

#endif // QSIGHTINGBUFFER_H
