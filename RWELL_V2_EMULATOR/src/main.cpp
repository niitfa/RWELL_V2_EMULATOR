#include "mainwindow/mainwindow.h"
#include <QApplication>
#include <string>
#include <QHostAddress>

bool validIP(std::string ip)
{
    QHostAddress ipv4addr( QString::fromStdString( ip ) );
    return QAbstractSocket::IPv4Protocol == ipv4addr.protocol();
}

int main(int argc, char *argv[])
{
    std::string ip = "0.0.0.0";
    uint16_t port = 22250;

    if(argc > 1)
    {
        if(validIP(argv[1]))
        {
           ip = argv[1];
        }
    }
    if(argc > 2)
    {
        QString qstrPort = argv[2];

        int newPort = qstrPort.toInt();
        if(newPort > 0 && newPort < 65535)
        {
            port = static_cast<uint16_t>(newPort);
        }
    }

    QApplication a(argc, argv);
    MainWindow w;
    w.createEmulator(ip, port);
    w.show();

    return a.exec();
}
