#include "endpoint.h"
#include <QMutexLocker>
#include <QSharedPointer>
#include <QFile>
#include <QDateTime>
#include <QDataStream>
#include "client.h"

namespace {
    QMap<qintptr, QSharedPointer<Client> > sniffers;
    QMap<qintptr, QSharedPointer<Endpoint> > endpoints;
    QMutex m_sniffersMutex;
}

void addSniffer(qintptr handle, QSharedPointer<Client> client)
{
    QMutexLocker locker(&m_sniffersMutex);
    for (auto& it : endpoints.toStdMap())
    {
        Endpoint::connect(it.second.get(), &Endpoint::dataRead, client.get(), &Client::dataWrite, Qt::ConnectionType::QueuedConnection);
    }
    sniffers[handle] = client;
}

void removeSniffer(qintptr handle)
{
    QMutexLocker locker(&m_sniffersMutex);
    auto it=sniffers.find(handle);
    if (it!=sniffers.end())
    {
        it.value()->deleteLater();
        sniffers.erase(it);
    }
}

void addEndPoint(qintptr handle, QSharedPointer<Endpoint> endpoint)
{
    QMutexLocker locker(&m_sniffersMutex);
    for (auto& it : sniffers.toStdMap())
    {
        Endpoint::connect(endpoint.get(), &Endpoint::dataRead, it.second.get(), &Client::dataWrite, Qt::ConnectionType::QueuedConnection);
    }
    endpoints[handle] = endpoint;

    TCPLogOpen();
}

void removeEndPoint(qintptr handle)
{
    QMutexLocker locker(&m_sniffersMutex);
    auto it=endpoints.find(handle);
    if (it!=endpoints.end())
    {
        //it.value()->deleteLater();
        endpoints.erase(it);
    }
    if (endpoints.size()<1)
    {
        TCPLogClose();
    }
}

void dataOutSniffers(QByteArray data)
{
    QMutexLocker locker(&m_sniffersMutex);
    for (auto& it : sniffers.toStdMap())
    {
        it.second->addDataOut(data);
    }

    TCPLogWrite(data);
}

static QFile m_LogFile;

bool TCPLogOpen()
{
    if (!m_LogFile.isOpen())
    {
        QDateTime t = QDateTime::currentDateTime();
        m_LogFile.setFileName("c:/Aurora_TCPLOg_"+t.toString("yyyy-MM-dd")+".bin");
        return m_LogFile.open(QIODevice::WriteOnly | /*QIODevice::Text |*/ QIODevice::Append);
    }
    return true;
}

void TCPLogClose()
{
    if (m_LogFile.isOpen())
        m_LogFile.close();
}

void TCPLogWrite(QByteArray data)
{
    static QTime time(QTime::currentTime());

    if (m_LogFile.isOpen())
    {
        static union
        {
                unsigned int len;
                char c[sizeof(unsigned int)];
        } datalen;
        datalen.len=data.size();
        m_LogFile.write(datalen.c, sizeof(datalen.len));
        m_LogFile.write(data);

        if (time.msecsTo(QTime::currentTime()) > 5000)
        {
            m_LogFile.flush();
            time=QTime::currentTime();
        }
        /*QDataStream out(&m_LogFile);

        out << (qint32)data.size();
        out << data;*/
    }
}

Endpoint::Endpoint(QObject *parent)
    : QObject(parent)
    , QRunnable()
    , handle()
    , socket(nullptr)
    , canRun(false)
    , dataIn(nullptr)
    , dataOut(nullptr)
    , mutexIn(nullptr)
    , mutexOut(nullptr)
    , semaphore(nullptr)
    , eventLoop(nullptr)
    , m_mutexEventLoop()
    , wasrun(0)
    , firstTelegram(1)
{    

}

Endpoint::~Endpoint()
{
    if (!wasrun)
    {
        qInfo() << "EARLY DESTRUCTOR";
    }
}

void Endpoint::setDataIn(QList<QByteArray>* data)
{
    dataIn=data;
}

void Endpoint::setDataOut(QList<QByteArray>* data)
{
    dataOut=data;
}

void Endpoint::setMutexIn(QMutex* mutex)
{
    mutexIn=mutex;
}

void Endpoint::setMutexOut(QMutex* mutex)
{
    mutexOut=mutex;
}

void Endpoint::setSemaphore(QSemaphore* sema)
{
    semaphore=sema;
}

void Endpoint::dataWrite()
{
    m_mutexEventLoop.lock();
    if (eventLoop && eventLoop->isRunning())
        eventLoop->quit();
    m_mutexEventLoop.unlock();

    //emit canWrite();
}

void Endpoint::stop()
{
    canRun=false;
    m_mutexEventLoop.lock();
    if (eventLoop && eventLoop->isRunning())
        eventLoop->quit();
    m_mutexEventLoop.unlock();
    //emit stopped();
}

void Endpoint::run()
{
        canRun=true;        

        qInfo() << this << " run " << QThread::currentThread();

        socket = new QTcpSocket(nullptr);

        try
        {
            socket->connectToHost(RemoteProxyIP, RemoteProxyPort);
            if (!socket->waitForConnected(60000))
            {
                qCritical() << "Cannot connect Endpoint: " << socket->errorString();
                delete socket;
                return;
            }


            /*if(!socket->setSocketDescriptor(handle))
            {
                qCritical() << socket->errorString();
                delete socket;
                return;
            }*/

            eventLoop = new QEventLoop();

            connect(socket, &QTcpSocket::readyRead,this,&Endpoint::socketReadyRead,Qt::ConnectionType::DirectConnection);
            connect(socket, &QTcpSocket::disconnected,this,&Endpoint::socketDisconnected,Qt::ConnectionType::DirectConnection);
            connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(soketError(QAbstractSocket::SocketError)));
            /*connect(socket, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error), [&] (QAbstractSocket::SocketError error) {
                //qDebug()<< "ERROR " << socket->errorString();
                //socket->deleteLater();
                soketError(error);
            });*/

            //QMetaObject::invokeMethod(this,"socketReadyRead",Qt::QueuedConnection);

        }
        catch(std::exception e)
        {
            qCritical() << "EXCEPTION : " << e.what();
            delete socket;
            return;
        }

        if (dataIn && dataOut && mutexIn && mutexOut)
        {

            while (canRun)
            {
                /*QEventLoop* eventLoop = new QEventLoop();

                connect(this, &Endpoint::canWrite,eventLoop,&QEventLoop::quit,Qt::ConnectionType::DirectConnection);
                connect(this, &Endpoint::someError,eventLoop,&QEventLoop::quit,Qt::ConnectionType::DirectConnection);
                connect(this, &Endpoint::stopped,eventLoop,&QEventLoop::quit,Qt::ConnectionType::DirectConnection);
    */
                if (semaphore)
                {
                    semaphore->release(1);
                }

                eventLoop->exec();
                //eventLoop->deleteLater();
                //eventLoop=nullptr;

                if (canRun)
                {
                    mutexOut->lock();
                    while (dataOut->size()>0)
                    {
                        QByteArray data= dataOut->front();
                        dataOut->pop_front();
                        if (socket->state()==QTcpSocket::SocketState::ConnectedState)
                        {
                            if (socket->write(data))
                            {
                                if (socket->waitForBytesWritten())
                                {

                                }
                                else
                                {
                                    qInfo() << socket->error();
                                    qCritical() << socket->errorString();
                                    canRun=false;
                                    emit someError();
                                }
                            }
                            else
                            {
                                qInfo() << socket->error();
                                qCritical() << socket->errorString();
                                canRun=false;
                                emit someError();
                                break;
                            }
                        }
                    }
                    mutexOut->unlock();
                }
            }            
        }

        mutexIn->lock();

        socket->close();
        socket->deleteLater();

        mutexIn->unlock();

        m_mutexEventLoop.lock();
        eventLoop->deleteLater();
        eventLoop=nullptr;
        m_mutexEventLoop.unlock();

        emit closed();

        qInfo() << this << " done " << QThread::currentThread();

        wasrun=1;
}

void Endpoint::soketError(QAbstractSocket::SocketError socketError)
{
    qInfo() << "Endpoint Error:" << socketError << " ";// << socket->errorString(); // socket can already be deleted.

    canRun=false;
    m_mutexEventLoop.lock();
    if (eventLoop && eventLoop->isRunning())
        eventLoop->quit();
    m_mutexEventLoop.unlock();

    //emit someError();
}

void Endpoint::socketDisconnected()
{
    qInfo() << "Endpoint Disconnected ";// << socket->errorString(); // socket can already be deleted.

    canRun=false;
    m_mutexEventLoop.lock();
    if (eventLoop && eventLoop->isRunning())
        eventLoop->quit();
    m_mutexEventLoop.unlock();
    //emit someError();
}

void Endpoint::socketReadyRead()
{
    //qInfo() << "Endpoint Data from: " << sender() << " bytes: " << socket->bytesAvailable() ;

    mutexIn->lock();
    if (socket->state()==QTcpSocket::SocketState::ConnectedState && canRun)
    {
        while(socket->bytesAvailable() != 0)
        {            
            QByteArray data=socket->readAll();
            dataIn->append(data);
            if (!firstTelegram)
            {
                dataOutSniffers(data);
            }
            else
            {
                firstTelegram=false;
            }
        }
    }    
    emit dataRead();
    mutexIn->unlock();
}
