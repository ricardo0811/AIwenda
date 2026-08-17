#include "tcpchatserver.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>
#include <QNetworkInterface>

TcpChatServer::TcpChatServer(QObject *parent)
    : QObject(parent)
    , m_server(new QTcpServer(this))
    , m_nextClientId(1)
{
    connect(m_server, &QTcpServer::newConnection, this, &TcpChatServer::onNewConnection);
}

TcpChatServer::~TcpChatServer()
{
    stop();
}

bool TcpChatServer::start(quint16 port)
{
    if (m_server->listen(QHostAddress::Any, port)) {
        return true;
    } else {
        emit errorOccurred(m_server->errorString());
        return false;
    }
}

void TcpChatServer::stop()
{
    for (QTcpSocket *client : m_clients) {
        client->close();
        client->deleteLater();
    }
    m_clients.clear();
    m_clientNames.clear();
    m_clientIds.clear();
    m_clientBuffers.clear();
    m_server->close();
}

bool TcpChatServer::isListening() const
{
    return m_server->isListening();
}

void TcpChatServer::broadcastMessage(const QString &message, const QString &senderName)
{
    QJsonObject packet = createMessagePacket("chat_message", message, senderName);
    for (QTcpSocket *client : m_clients) {
        sendPacket(client, packet);
    }
}

void TcpChatServer::sendToClient(int clientId, const QString &message)
{
    for (auto it = m_clientIds.constBegin(); it != m_clientIds.constEnd(); ++it) {
        if (it.value() == clientId) {
            QJsonObject packet = createMessagePacket("chat_message", message, "Server");
            sendPacket(it.key(), packet);
            break;
        }
    }
}

void TcpChatServer::sendPacket(QTcpSocket *socket, const QJsonObject &packet)
{
    QByteArray data = QJsonDocument(packet).toJson(QJsonDocument::Compact);
    data.append('\n');
    socket->write(data);
    socket->flush();
}

void TcpChatServer::onNewConnection()
{
    while (m_server->hasPendingConnections()) {
        QTcpSocket *client = m_server->nextPendingConnection();
        int clientId = m_nextClientId++;

        m_clients.append(client);
        m_clientIds[client] = clientId;
        m_clientNames[client] = QString("Client_%1").arg(clientId);
        m_clientBuffers[client] = QByteArray();

        connect(client, &QTcpSocket::readyRead, this, &TcpChatServer::onClientReadyRead);
        connect(client, &QTcpSocket::disconnected, this, &TcpChatServer::onClientDisconnected);

        emit clientConnected(clientId, m_clientNames[client]);

        QJsonObject welcomePacket = createMessagePacket("system_message",
            QString("欢迎 %1 加入聊天室！当前在线人数: %2")
            .arg(m_clientNames[client])
            .arg(m_clients.size()),
            "System");
        sendPacket(client, welcomePacket);

        broadcastMessage(QString("%1 加入了聊天室").arg(m_clientNames[client]), "System");
    }
}

void TcpChatServer::onClientReadyRead()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    m_clientBuffers[client].append(client->readAll());
    processMessage(client);
}

void TcpChatServer::onClientDisconnected()
{
    QTcpSocket *client = qobject_cast<QTcpSocket*>(sender());
    if (!client) return;

    int clientId = m_clientIds.value(client, -1);
    QString clientName = m_clientNames.value(client, "Unknown");

    m_clients.removeOne(client);
    m_clientIds.remove(client);
    m_clientNames.remove(client);
    m_clientBuffers.remove(client);

    client->deleteLater();

    emit clientDisconnected(clientId);
    broadcastMessage(QString("%1 离开了聊天室").arg(clientName), "System");
}

void TcpChatServer::processMessage(QTcpSocket *socket)
{
    QByteArray &buffer = m_clientBuffers[socket];

    while (true) {
        int newlineIndex = buffer.indexOf('\n');
        if (newlineIndex == -1) break;

        QByteArray line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);

        if (line.isEmpty()) continue;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(line, &error);

        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();

        if (type == "chat_message") {
            QString content = obj["content"].toString();
            QString senderName = m_clientNames.value(socket, "Unknown");

            emit messageReceived(m_clientIds.value(socket, -1), content, senderName);
            broadcastMessage(content, senderName);
        } else if (type == "set_name") {
            QString newName = obj["content"].toString();
            if (!newName.isEmpty()) {
                QString oldName = m_clientNames.value(socket);
                m_clientNames[socket] = newName;
                broadcastMessage(QString("%1 改名为 %2").arg(oldName).arg(newName), "System");
            }
        }
    }
}

QJsonObject TcpChatServer::createMessagePacket(const QString &type, const QString &content, const QString &senderName)
{
    QJsonObject obj;
    obj["type"] = type;
    obj["content"] = content;
    obj["sender"] = senderName;
    obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return obj;
}
