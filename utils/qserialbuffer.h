#ifndef QSERIALBUFFER_H
#define QSERIALBUFFER_H

#include <QObject>
#include <QQueue>

class QSerialBuffer: public QObject {
    Q_OBJECT
private:
    QByteArray m_data;
public:
    QSerialBuffer(QObject * parent = nullptr);
    void insert(const QByteArray & bytes);
signals:
    void message_complete(const QByteArray & message);
};

#endif // QSERIALBUFFER_H
