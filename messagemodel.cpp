#include "messagemodel.h"
#include <QDateTime>
#include <QUuid>

QJsonObject ChatMessage::toJson() const
{
    QJsonObject obj;
    obj["id"] = id;
    obj["role"] = role;
    obj["content"] = content;
    obj["timestamp"] = timestamp;
    obj["is_ai"] = isAI;
    obj["sentiment"] = sentiment;
    obj["translation"] = translation;
    obj["sender_name"] = senderName;
    return obj;
}

ChatMessage ChatMessage::fromJson(const QJsonObject &obj)
{
    ChatMessage msg;
    msg.id = obj["id"].toString();
    msg.role = obj["role"].toString();
    msg.content = obj["content"].toString();
    msg.timestamp = obj["timestamp"].toString();
    msg.isAI = obj["is_ai"].toBool();
    msg.sentiment = obj["sentiment"].toString("neutral");
    msg.translation = obj["translation"].toString();
    msg.senderName = obj["sender_name"].toString();
    return msg;
}

MessageModel::MessageModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MessageModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_messages.count();
}

QVariant MessageModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() >= m_messages.count())
        return QVariant();

    const ChatMessage &msg = m_messages.at(index.row());

    switch (role) {
    case IdRole: return msg.id;
    case RoleRole: return msg.role;
    case ContentRole: return msg.content;
    case TimestampRole: return msg.timestamp;
    case IsAIRole: return msg.isAI;
    case SentimentRole: return msg.sentiment;
    case TranslationRole: return msg.translation;
    case SenderNameRole: return msg.senderName;
    default: return QVariant();
    }
}

QHash<int, QByteArray> MessageModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[RoleRole] = "role";
    roles[ContentRole] = "content";
    roles[TimestampRole] = "timestamp";
    roles[IsAIRole] = "isAI";
    roles[SentimentRole] = "sentiment";
    roles[TranslationRole] = "translation";
    roles[SenderNameRole] = "senderName";
    return roles;
}

void MessageModel::addMessage(const ChatMessage &message)
{
    beginInsertRows(QModelIndex(), m_messages.count(), m_messages.count());
    m_messages.append(message);
    endInsertRows();
    emit countChanged();
    emit messageAdded(message);
}

void MessageModel::updateMessageContent(const QString &id, const QString &content)
{
    for (int i = 0; i < m_messages.count(); ++i) {
        if (m_messages[i].id == id) {
            m_messages[i].content = content;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {ContentRole});
            emit messageUpdated(id);
            return;
        }
    }
}

void MessageModel::updateMessageSentiment(const QString &id, const QString &sentiment)
{
    for (int i = 0; i < m_messages.count(); ++i) {
        if (m_messages[i].id == id) {
            m_messages[i].sentiment = sentiment;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {SentimentRole});
            emit messageUpdated(id);
            return;
        }
    }
}

void MessageModel::updateMessageTranslation(const QString &id, const QString &translation)
{
    for (int i = 0; i < m_messages.count(); ++i) {
        if (m_messages[i].id == id) {
            m_messages[i].translation = translation;
            QModelIndex idx = index(i);
            emit dataChanged(idx, idx, {TranslationRole});
            emit messageUpdated(id);
            return;
        }
    }
}

void MessageModel::clear()
{
    beginResetModel();
    m_messages.clear();
    endResetModel();
    emit countChanged();
}

QList<ChatMessage> MessageModel::messages() const
{
    return m_messages;
}

QList<ChatMessage> MessageModel::recentMessages(int count) const
{
    int start = qMax(0, m_messages.count() - count);
    return m_messages.mid(start);
}

QJsonArray MessageModel::toJsonArray() const
{
    QJsonArray arr;
    for (const ChatMessage &msg : m_messages) {
        arr.append(msg.toJson());
    }
    return arr;
}

void MessageModel::fromJsonArray(const QJsonArray &array)
{
    beginResetModel();
    m_messages.clear();
    for (const QJsonValue &val : array) {
        if (val.isObject()) {
            m_messages.append(ChatMessage::fromJson(val.toObject()));
        }
    }
    endResetModel();
    emit countChanged();
}

ChatMessage MessageModel::messageAt(int index) const
{
    if (index >= 0 && index < m_messages.count()) {
        return m_messages.at(index);
    }
    return ChatMessage();
}

ChatMessage MessageModel::lastMessage() const
{
    if (!m_messages.isEmpty()) {
        return m_messages.last();
    }
    return ChatMessage();
}

ChatMessage MessageModel::getMessage(const QString &id) const
{
    for (const ChatMessage &msg : m_messages) {
        if (msg.id == id) {
            return msg;
        }
    }
    return ChatMessage();
}

void MessageModel::removeMessage(const QString &id)
{
    for (int i = 0; i < m_messages.count(); ++i) {
        if (m_messages[i].id == id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_messages.removeAt(i);
            endRemoveRows();
            emit countChanged();
            return;
        }
    }
}
