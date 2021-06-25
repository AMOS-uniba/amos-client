#ifndef QCAMERA_H
#define QCAMERA_H

#include <QGroupBox>

namespace Ui {
    class QCamera;
}

class QCamera: public QGroupBox {
    Q_OBJECT
private:
    Ui::QCamera * ui;

public:
    explicit QCamera(QWidget * parent = nullptr);
    ~QCamera();
};

#endif // QCAMERA_H
