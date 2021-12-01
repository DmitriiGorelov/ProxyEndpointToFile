#include <QCoreApplication>
#include "manager.h"
#include "sniffer.h"

// High performance multi-threaded TCP server

// Test with Siege
// https://www.joedog.org/siege-home/

// siege -c 1 127.0.0.1:2020
// siege -c 10 127.0.0.1:2020
// siege -c 100 127.0.0.1:2020

#include "definitions.h"

#include <QNetworkProxyFactory>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QNetworkProxyFactory::setUseSystemConfiguration(false); // to avoid self-loop of proxy if proxy is configured in the system to this clientPort

    QThreadPool::globalInstance()->setMaxThreadCount(200);

    Manager::Instance()->start(AuroraPort);

    Sniffer::Instance()->start(snifferPort);

    return a.exec();
}
