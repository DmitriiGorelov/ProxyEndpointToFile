#include "sniffer.h"
#include <QtConcurrent/QtConcurrent>
#include <QSharedPointer>
#include "eventloophelper.h"
#include "endpoint.h"

Sniffer* Sniffer::sniffer;

Sniffer::Sniffer(QObject *parent)
    : QTcpServer(parent)
{
}

void Sniffer::start(quint16 port)
{
    qInfo() << this << " start " << QThread::currentThread();
    if(this->listen(QHostAddress::Any, port))
    {
        qInfo() << "Server started on " << port;
    }
    else
    {
        qCritical() << this->errorString();
    }
}

void Sniffer::quit()
{
    this->close();
    qInfo() << "Server Stopped!";
}

void Sniffer::newSniffer(qintptr handle)
{
    QList<QByteArray> dataClient;
    QList<QByteArray> dataServer;
    QMutex mutexClient;
    QMutex mutexServer;
    QSemaphore sema;

    QSharedPointer<Client> client(new Client(nullptr, handle)); // no parent for other thread!

    //connect(client, &Client::dataRead, endpoint, &Endpoint::dataWrite, Qt::ConnectionType::QueuedConnection);

    //connect(client, &Client::closed, endpoint, &Endpoint::stop, Qt::ConnectionType::QueuedConnection);

    client->setDataIn(&dataClient);
    client->setDataOut(&dataServer);
    client->setMutexIn(&mutexClient);
    client->setMutexOut(&mutexServer);
    client->setSemaphore(&sema);

    client->setAutoDelete(false);

    QThreadPool pool;// = new QThreadPool(nullptr);
    pool.setMaxThreadCount(2);

    sema.release();

    addSniffer(handle, client);

    pool.start(client.get());

    //QEventLoop* eventLoop1 = new QEventLoop(nullptr);
    EventLoopHelper* eventLoop = new EventLoopHelper(nullptr,1);

    connect(client.get(), &Client::closed,eventLoop,&EventLoopHelper::inc,Qt::ConnectionType::DirectConnection);

    eventLoop->exec();

    pool.waitForDone();

    delete eventLoop;

    removeSniffer(handle);

    qInfo() << "                 Sniffer Exit!";
}

void Sniffer::incomingConnection(qintptr handle)
{
    QFuture<void> future = QtConcurrent::run(Sniffer::newSniffer,handle);
}
