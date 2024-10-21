#ifndef QCAMERA_H
#define QCAMERA_H

#include <QGroupBox>

#include "widgets/qconfigurable.h"
#include "utils/sighting.h"

QT_FORWARD_DECLARE_CLASS(QStation);

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
    bool m_spectral;

    double m_darkness_limit;

    constexpr static qint64 DeferTime = 30;            // Time in seconds: how long to defer an unsent sighting
    QMap<QString, QDateTime> m_deferred_sightings;

    void connect_slots(void) override;
    void load_defaults(void) override;
    void load_settings_inner(void) override;
    void save_settings_inner(void) const override;
    void apply_changes_inner(void) override;
    void discard_changes_inner(void) override;
    using QAmosWidget::initialize;

    inline QString enabled_key(void) const { return QString("camera_%1/enabled").arg(this->id()); }
    inline QString darkness_key(void) const { return QString("camera_%1/darkness_limit").arg(this->id()); }

    constexpr static double DefaultDarknessLimit = -12.0;
    constexpr static bool DefaultEnabled = true;

public:
    explicit QCamera(QWidget * parent = nullptr);
    ~QCamera();

    bool is_changed(void) const override;

    inline const QString & id(void) const { return this->m_id; }
    inline bool is_enabled(void) const { return this->m_enabled; }
    inline bool is_spectral(void) const { return this->m_spectral; }
    inline double darkness_limit(void) const { return this->m_darkness_limit; }

    QJsonObject json(void) const;

private slots:
    void set_enabled(int enable);
    void set_darkness_limit(double new_limit);

    void process_sightings(QVector<Sighting> sightings);

public slots:
    void initialize(QSettings * settings, const QString & id, const QStation * const station, bool spectral);
    void auto_action(bool is_dark, const QDateTime & open_since = QDateTime());
    void update_clocks(void);

    void store_sighting(const QString & sighting_id);
    void discard_sighting(const QString & sighting_id);
    void defer_sighting(const QString & sighting_id);

signals:
    void darkness_limit_changed(double new_limit);

    void sighting_found(Sighting & sightings);
    void sightings_stored(QVector<Sighting> & sightings);
};

#endif // QCAMERA_H
