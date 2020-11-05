#ifndef SERIALBUFFER_H
#define SERIALBUFFER_H

#include <QObject>
#include <QQueue>

class SerialBuffer: public QObject {
    Q_OBJECT
private:
    QByteArray m_data;
public:
    SerialBuffer(void);
    void insert(const QByteArray &bytes);
signals:
    void message_complete(const QByteArray &message);
};

#endif // SERIALBUFFER_H
