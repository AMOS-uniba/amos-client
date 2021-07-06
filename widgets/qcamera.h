#ifndef QCAMERA_H
#define QCAMERA_H

#include <QGroupBox>

#include "widgets/qconfigurable.h"
#include "utils/sighting.h"

namespace Ui {
    class QCamera;
}

class QCamera: public QAmosWidget {
    Q_OBJECT
private:
    Ui::QCamera * ui;
    const QStation * m_station;
    QString m_id;

    double m_darkness_limit;

    void connect_slots(void) override;
    void load_defaults(void) override;
    void load_settings_inner(const QSettings * const settings) override;
    void save_settings_inner(QSettings * settings) const override;
    void apply_changes_inner(void) override;
    void discard_changes_inner(void) override;
    using QAmosWidget::initialize;

public:
    explicit QCamera(QWidget * parent = nullptr);
    ~QCamera();

    bool is_changed(void) const override;

    inline const QString & id(void) const { return this->m_id; }
    inline double darkness_limit(void) const { return this->m_darkness_limit; }

    QJsonObject json(void) const;

    const QUfoManager * ufo_manager(void) const;

private slots:
    void set_darkness_limit(double new_limit);

    void store_sightings(QVector<Sighting> sightings);

public slots:
    void initialize(const QString & id, const QStation * const station);
    void update_clocks(void);

signals:
    void darkness_limit_changed(double new_limit) const;

    void sightings_found(QVector<Sighting> sightings) const;
};

#endif // QCAMERA_H
