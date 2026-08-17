#include "tcpchatclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDateTime>

TcpChatClient::TcpChatClient(QObject *parent)
    : QObject(parent)
    , m_socket(new QTcpSocket(this))
{
    connect(m_socket, &QTcpSocket::connected, this, &TcpChatClient::onConnected);
    connect(m_socket, &QTcpSocket::disconnected, this, &TcpChatClient::onDisconnected);
    connect(m_socket, &QTcpSocket::readyRead, this, &TcpChatClient::onReadyRead);
    connect(m_socket, &QTcpSocket::errorOccurred, this, &TcpChatClient::onError);
}

TcpChatClient::~TcpChatClient()
{
    disconnect();
}

void TcpChatClient::connectToServer(const QString &host, quint16 port)
{
    if (m_socket->state() == QAbstractSocket::ConnectedState) {
        m_socket->disconnectFromHost();
    }
    m_buffer.clear();
    m_socket->connectToHost(host, port);
}

void TcpChatClient::disconnect()
{
    if (m_socket->state() != QAbstractSocket::UnconnectedState) {
        m_socket->disconnectFromHost();
    }
}

bool TcpChatClient::isConnected() const
{
    return m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpChatClient::sendMessage(const QString &message)
{
    if (!isConnected()) {
        emit errorOccurred("未连接到服务器");
        return;
    }

    QJsonObject packet = createMessagePacket("chat_message", message);
    sendPacket(packet);
}

void TcpChatClient::setName(const QString &name)
{
    m_userName = name;
    if (isConnected()) {
        QJsonObject packet = createMessagePacket("set_name", name);
        sendPacket(packet);
    }
}

void TcpChatClient::sendPacket(const QJsonObject &packet)
{
    QByteArray data = QJsonDocument(packet).toJson(QJsonDocument::Compact);
    data.append('\n');
    m_socket->write(data);
    m_socket->flush();
}

void TcpChatClient::onConnected()
{
    emit connected();
    if (!m_userName.isEmpty()) {
        setName(m_userName);
    }
}

void TcpChatClient::onDisconnected()
{
    emit disconnected();
}

void TcpChatClient::onReadyRead()
{
    m_buffer.append(m_socket->readAll());
    processBuffer();
}

void TcpChatClient::processBuffer()
{
    while (true) {
        int newlineIndex = m_buffer.indexOf('\n');
        if (newlineIndex == -1) break;

        QByteArray line = m_buffer.left(newlineIndex).trimmed();
        m_buffer.remove(0, newlineIndex + 1);

        if (line.isEmpty()) continue;

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(line, &error);

        if (error.error != QJsonParseError::NoError || !doc.isObject()) {
            continue;
        }

        QJsonObject obj = doc.object();
        QString type = obj["type"].toString();
        QString content = obj["content"].toString();
        QString sender = obj["sender"].toString();

        if (type == "chat_message") {
            emit messageReceived(content, sender);
        } else if (type == "system_message") {
            emit systemMessageReceived(content);
        }
    }
}

void TcpChatClient::onError(QAbstractSocket::SocketError error)
{
    Q_UNUSED(error)
    QString errStr = m_socket->errorString();
    if (!errStr.contains("canceled", Qt::CaseInsensitive)
        && !errStr.contains("取消", Qt::CaseInsensitive)
        && !m_socket->state() == QAbstractSocket::UnconnectedState) {
        emit errorOccurred(errStr);
    }
}

QJsonObject TcpChatClient::createMessagePacket(const QString &type, const QString &content)
{
    QJsonObject obj;
    obj["type"] = type;
    obj["content"] = content;
    obj["sender"] = m_userName.isEmpty() ? "Anonymous" : m_userName;
    obj["timestamp"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    return obj;
}
