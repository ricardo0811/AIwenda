#ifndef TCPCHATCLIENT_H
#define TCPCHATCLIENT_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>

class TcpChatClient : public QObject
{
    Q_OBJECT

public:
    explicit TcpChatClient(QObject *parent = nullptr);
    ~TcpChatClient() override;

    void connectToServer(const QString &host, quint16 port = 8888);
    void disconnect();
    bool isConnected() const;

    void sendMessage(const QString &message);
    void setName(const QString &name);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString &message, const QString &senderName);
    void systemMessageReceived(const QString &message);
    void errorOccurred(const QString &errorMessage);

private slots:
    void onConnected();
    void onDisconnected();
    void onReadyRead();
    void onError(QAbstractSocket::SocketError error);

private:
    QTcpSocket *m_socket;
    QString m_userName;
    QByteArray m_buffer;

    QJsonObject createMessagePacket(const QString &type, const QString &content);
    void sendPacket(const QJsonObject &packet);
    void processBuffer();
};

#endif // TCPCHATCLIENT_H
