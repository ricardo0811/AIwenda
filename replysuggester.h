#ifndef REPLYSUGGESTER_H
#define REPLYSUGGESTER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QStringList>
#include <QJsonObject>
#include <QJsonArray>

class ReplySuggester : public QObject
{
    Q_OBJECT

public:
    explicit ReplySuggester(QObject *parent = nullptr);
    ~ReplySuggester() override;

    void suggest(const QString &message, int count = 3);
    void cancel();

signals:
    void suggestionsReady(const QStringList &suggestions, const QString &originalMessage);
    void errorOccurred(const QString &errorMessage);

private slots:
    void onReplyFinished();
    void onReplyError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_currentReply;
    QString m_pendingMessage;
    int m_suggestCount;

    QJsonObject buildSuggestRequest(const QString &message, int count);
    QStringList parseSuggestions(const QString &response);
};

#endif // REPLYSUGGESTER_H
