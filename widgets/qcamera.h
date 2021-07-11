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
    bool m_enabled;

    double m_darkness_limit;

    void connect_slots(void) override;
    void load_defaults(void) override;
    void load_settings_inner(const QSettings * const settings) override;
    void save_settings_inner(QSettings * settings) const override;
    void apply_changes_inner(void) override;
    void discard_changes_inner(void) override;
    using QAmosWidget::initialize;

    inline QString enabled_key(void) const { return QString("camera_%1/enabled").arg(this->id()); }
    inline QString darkness_key(void) const { return QString("camera_%1/darkness_limit").arg(this->id()); }

public:
    explicit QCamera(QWidget * parent = nullptr);
    ~QCamera();

    bool is_changed(void) const override;

    inline const QString & id(void) const { return this->m_id; }
    inline bool is_enabled(void) const { return this->m_enabled; }
    inline double darkness_limit(void) const { return this->m_darkness_limit; }

    QJsonObject json(void) const;

    const QUfoManager * ufo_manager(void) const;

private slots:
    void set_enabled(int enable);
    void set_darkness_limit(double new_limit);

    void store_sightings(QVector<Sighting> sightings);

public slots:
    void initialize(const QString & id, const QStation * const station);
    void update_clocks(void);

signals:
    void darkness_limit_changed(double new_limit) const;

    void sightings_found(QVector<Sighting> sightings) const;
    void sightings_stored(QVector<Sighting> sightings) const;
};

#endif // QCAMERA_H
