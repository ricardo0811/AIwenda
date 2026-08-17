#ifndef TCPCHATSERVER_H
#define TCPCHATSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QList>
#include <QMap>
#include <QJsonObject>

class TcpChatServer : public QObject
{
    Q_OBJECT

public:
    explicit TcpChatServer(QObject *parent = nullptr);
    ~TcpChatServer() override;

    bool start(quint16 port = 8888);
    void stop();
    bool isListening() const;

    void broadcastMessage(const QString &message, const QString &senderName);
    void sendToClient(int clientId, const QString &message);

signals:
    void clientConnected(int clientId, const QString &clientName);
    void clientDisconnected(int clientId);
    void messageReceived(int clientId, const QString &message, const QString &senderName);
    void errorOccurred(const QString &errorMessage);

private slots:
    void onNewConnection();
    void onClientReadyRead();
    void onClientDisconnected();

private:
    QTcpServer *m_server;
    QList<QTcpSocket*> m_clients;
    QMap<QTcpSocket*, QString> m_clientNames;
    QMap<QTcpSocket*, int> m_clientIds;
    QMap<QTcpSocket*, QByteArray> m_clientBuffers;
    int m_nextClientId;

    void processMessage(QTcpSocket *socket);
    QJsonObject createMessagePacket(const QString &type, const QString &content, const QString &senderName);
    void sendPacket(QTcpSocket *socket, const QJsonObject &packet);
};

#endif // TCPCHATSERVER_H
