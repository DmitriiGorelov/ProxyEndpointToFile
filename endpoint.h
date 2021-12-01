#ifndef ENDPOINT_H
#define ENDPOINT_H

#include <QObject>
#include <QDebug>
#include <QRunnable>
#include <QThread>
#include <QTcpSocket>
#include <QEventLoop>
#include <QList>
#include <QMutex>
#include <QByteArray>
#include <QSemaphore>
#include <QAtomicInt>
#include "definitions.h"

class Client;

class Endpoint : public QObject, public QRunnable
{
    Q_OBJECT
public:
    explicit Endpoint(QObject *parent = nullptr);
    ~Endpoint();
    void setDataIn(QList<QByteArray>* data);
    void setDataOut(QList<QByteArray>* data);
    void setMutexIn(QMutex* mutex);
    void setMutexOut(QMutex* mutex);
    void setSemaphore(QSemaphore* sema);

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
    QList<QByteArray>* dataIn;
    QList<QByteArray>* dataOut;
    QMutex* mutexIn;
    QMutex* mutexOut;
    QSemaphore* semaphore;
    QEventLoop* eventLoop;
    QMutex m_mutexEventLoop;
    QAtomicInt wasrun;
    QAtomicInt firstTelegram;
};

void dataOutSniffers(QByteArray data);
void addSniffer(qintptr handle, QSharedPointer<Client> client);
void removeSniffer(qintptr handle);
void addEndPoint(qintptr handle, QSharedPointer<Endpoint> endpoint);
void removeEndPoint(qintptr handle);
bool TCPLogOpen();
void TCPLogClose();
void TCPLogWrite(QByteArray data);
#endif // ENDPOINT_H
