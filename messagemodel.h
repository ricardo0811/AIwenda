#ifndef MESSAGEMODEL_H
#define MESSAGEMODEL_H

#include <QObject>
#include <QAbstractListModel>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>

struct ChatMessage {
    ChatMessage() : isAI(false), sentiment("neutral") {}

    QString id;
    QString role;       // "user", "assistant", "system"
    QString content;
    QString timestamp;
    bool isAI;
    QString sentiment;  // "positive", "negative", "neutral"
    QString translation;
    QString senderName;

    QJsonObject toJson() const;
    static ChatMessage fromJson(const QJsonObject &obj);
};

class MessageModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        RoleRole,
        ContentRole,
        TimestampRole,
        IsAIRole,
        SentimentRole,
        TranslationRole,
        SenderNameRole
    };

    explicit MessageModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void addMessage(const ChatMessage &message);
    void updateMessageContent(const QString &id, const QString &content);
    void updateMessageSentiment(const QString &id, const QString &sentiment);
    void updateMessageTranslation(const QString &id, const QString &translation);
    void removeMessage(const QString &id);
    void clear();

    QList<ChatMessage> messages() const;
    QList<ChatMessage> recentMessages(int count) const;
    QJsonArray toJsonArray() const;
    void fromJsonArray(const QJsonArray &array);

    ChatMessage messageAt(int index) const;
    ChatMessage lastMessage() const;
    ChatMessage getMessage(const QString &id) const;

signals:
    void countChanged();
    void messageAdded(const ChatMessage &message);
    void messageUpdated(const QString &id);

private:
    QList<ChatMessage> m_messages;
    int m_maxContextMessages = 20;
};

#endif // MESSAGEMODEL_H
