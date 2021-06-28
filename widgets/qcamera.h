#ifndef QCAMERA_H
#define QCAMERA_H

#include <QGroupBox>

#include "utils/sighting.h"

namespace Ui {
    class QCamera;
}

class QCamera: public QGroupBox {
    Q_OBJECT
private:
    Ui::QCamera * ui;
    QString m_id;

    double m_darkness_limit;

    void load_settings(const QSettings * const settings);
    void load_settings_inner(const QSettings * const settings);
    void apply_changes_inner(void);

    bool is_changed(void) const;
    bool is_darkness_limit_changed(void) const;

public:
    explicit QCamera(QWidget * parent = nullptr);
    ~QCamera();

    inline const QString & id(void) const { return this->m_id; }
    inline double darkness_limit(void) const { return this->m_darkness_limit; }

    void initialize(const QString & id);

    QJsonObject json(void) const;

private slots:
    void load_defaults(void);
    void save_settings(void) const;

    void apply_changes(void);
    void discard_changes(void);
    void handle_config_changed(void);

    void set_darkness_limit(double new_limit);

public slots:
    void store_sightings(QVector<Sighting> sightings);

signals:
    void settings_changed(void) const;
    void settings_discarded(void) const;

    void darkness_limit_changed(double new_limit) const;

    void sightings_found(QVector<Sighting> sightings) const;
};

#endif // QCAMERA_H
