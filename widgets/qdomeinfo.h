#ifndef QDOMEINFO_H
#define QDOMEINFO_H

#include <QGroupBox>

namespace Ui {
class QDomeInfo;
}

class QDomeInfo : public QGroupBox
{
    Q_OBJECT

public:
    explicit QDomeInfo(QWidget *parent = nullptr);
    ~QDomeInfo();

private:
    Ui::QDomeInfo *ui;
};

#endif // QDOMEINFO_H
