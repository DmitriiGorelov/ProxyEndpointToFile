#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QDebug>
#include <QRunnable>
#include <QThread>
#include <QTcpSocket>
#include <QEventLoop>
#include <QList>
#include <QMutex>
#include <QMutexLocker>
#include <QByteArray>
#include <QSemaphore>
#include <QAtomicInt>

#include "definitions.h"

//Q_DECLARE_METATYPE( QAbstractSocket::SocketError); // в .h

class Client : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr, qintptr _handle = 0);

    void setDataIn(QList<QByteArray>* data);
    void setDataOut(QList<QByteArray>* data);
    void setMutexIn(QMutex* mutex);
    void setMutexOut(QMutex* mutex);
    void setSemaphore(QSemaphore* sema);

    QString name();
    void name(const QString& value);

    void addDataOut(QByteArray data);

signals:
    void closed();
    void dataRead();
    void canWrite();
    void someError();

public slots:
    void dataWrite();
    void stop();

private slots:
    void soketError(QAbstractSocket::SocketError socketError);
    void socketReadyRead();
    void socketDisconnected();

private: signals:
    void stopped();

public:
    void run() final;

private:
    qintptr handle;
    QTcpSocket* socket;
    QAtomicInt canRun;
    QAtomicInt firstDataIn;
    QList<QByteArray>* dataIn;
    QList<QByteArray>* dataOut;
    QMutex* mutexIn;
    QMutex* mutexOut;
    QSemaphore* semaphore;
    QString m_name;
    QMutex m_mutexName;
    QEventLoop* eventLoop;
    QMutex m_mutexEventLoop;
};

#endif // CLIENT_H
