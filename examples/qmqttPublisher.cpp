#include <QObject>
#include <QCoreApplication>
#include <QCommandLineParser>
#include <QTimer>

#include <qmqtt_client.h>

using QMQTT::Client;
using QMQTT::Message;
using QMQTT::Will;

class Logger : public QObject {
    public:
        Logger(QObject * parent = 0) : QObject(0) {
        }
    public slots:
        void showError(QAbstractSocket::SocketError error) {
            qDebug() << "error" << error;
        }
        void showDisconnected() {
            qDebug() << "disconnected";
        }
        void showPublished () {
            qDebug() << "published";
        }
        void showConnected() {
            qDebug() << "connected";
        }
};

class MyClient : public Client {
    public:
    MyClient(const QString & host, quint32 port) : Client(host, port) {
    }
    void sendMessage() {
        publish(toPublish);
    }

    void setMessage(const Message & msg) {
        toPublish = msg;
    }
    private:
    Message toPublish;
};

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);
    QCommandLineParser parser;
    QCommandLineOption hostOption("host",
            QCoreApplication::translate("host", "The MQTT host to connect, localhost used if not defined."),
            "host", "localhost");
    QCommandLineOption qosOption("qos",
            QCoreApplication::translate("qos", "Quality of Service level, 0 used if not defined."),
            "qos", "0");
    QCommandLineOption portOption("port",
            QCoreApplication::translate("port", "The MQTT port to connect, 1883 used if not defined."),
            "port", "1883");

    parser.addPositionalArgument("topic", QCoreApplication::translate("main", "Topic to subscribe"));
    parser.addPositionalArgument("msg", QCoreApplication::translate("message", "The message to publish."));

    parser.addOption(hostOption);
    parser.addOption(qosOption);
    parser.addOption(portOption);
    parser.process(app);
    QStringList args = parser.positionalArguments();

    if (args.size() < 2) {
        parser.showHelp(0);
        return 0;
    }

    int id = qrand();
    QString host = parser.value("host");
    quint32 port = parser.value("port").toUInt();
    QString qos = parser.value("qos");
    QString topic = args.at(0);
    QString message = args.at(1);
    Message msg(id, topic, message.toUtf8());
    MyClient client(host, port);
    Logger logger;
    QTimer timer;

    QObject::connect(&client, &MyClient::connected, &logger, &Logger::showConnected);
    QObject::connect(&client, &MyClient::published, &logger, &Logger::showPublished);
    QObject::connect(&client, &MyClient::disconnected, &logger, &Logger::showDisconnected);
    QObject::connect(&client, &MyClient::error, &logger, &Logger::showError);
    QObject::connect(&timer, &QTimer::timeout, &client, &MyClient::sendMessage);
    QObject::connect(&client, SIGNAL(connected()), &timer, SLOT(start()));
    QObject::connect(&client, SIGNAL(disconnected()), &timer , SLOT(stop()));
    QObject::connect(&app, SIGNAL(aboutToQuit()), &client, SLOT(disconnect()));

    qDebug() << "host" << host << "port" << port << "qos" << qos << "and message" << message;
    qDebug() << "topic" << topic << " with message" << message;

    client.setMessage(msg);
    client.connect();
    timer.setInterval(1000);

    return app.exec();
}
