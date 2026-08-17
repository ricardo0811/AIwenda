#include "replysuggester.h"
#include "apiconfig.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

ReplySuggester::ReplySuggester(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_suggestCount(3)
{
}

ReplySuggester::~ReplySuggester()
{
    cancel();
}

void ReplySuggester::suggest(const QString &message, int count)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    ApiConfig &config = ApiConfig::instance();

    m_pendingMessage = message;
    m_suggestCount = count;
    QJsonObject body = buildSuggestRequest(message, count);

    QUrl url(config.apiEndpoint());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey()).toUtf8());

    m_currentReply = m_manager->post(request, QJsonDocument(body).toJson());
    connect(m_currentReply, &QNetworkReply::finished, this, &ReplySuggester::onReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &ReplySuggester::onReplyError);
}

void ReplySuggester::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void ReplySuggester::onReplyFinished()
{
    if (!m_currentReply) return;

    if (m_currentReply->error() == QNetworkReply::NoError) {
        QByteArray responseData = m_currentReply->readAll();
        QJsonDocument doc = QJsonDocument::fromJson(responseData);

        if (doc.isObject()) {
            QJsonObject obj = doc.object();
            QJsonArray choices = obj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject choice = choices.first().toObject();
                QJsonObject messageObj = choice["message"].toObject();
                QString content = messageObj["content"].toString();
                QStringList suggestions = parseSuggestions(content);
                emit suggestionsReady(suggestions, m_pendingMessage);
            }
        }
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void ReplySuggester::onReplyError(QNetworkReply::NetworkError error)
{
    if (error != QNetworkReply::NetworkError::OperationCanceledError) {
        emit errorOccurred(m_currentReply ? m_currentReply->errorString() : "建议请求失败");
    }
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

QJsonObject ReplySuggester::buildSuggestRequest(const QString &message, int count)
{
    ApiConfig &config = ApiConfig::instance();

    QJsonObject body;
    body["model"] = config.model();

    QJsonArray messages;

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = QString("你是一个聊天助手。请根据对方的消息，生成%1条简短的回复建议。每条建议一行，不要编号，不要解释。").arg(count);
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = QString("对方说：\"%1\"\n请给出%2条回复建议：").arg(message).arg(count);
    messages.append(userMsg);

    body["messages"] = messages;

    return body;
}

QStringList ReplySuggester::parseSuggestions(const QString &response)
{
    QStringList suggestions;
    QStringList lines = response.split('\n', Qt::SkipEmptyParts);

    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty()) continue;

        QRegularExpression numRegex("^\\d+[.、)）]\\s*");
        line.remove(numRegex);

        QRegularExpression bulletRegex("^[-*•]\\s*");
        line.remove(bulletRegex);

        if (!line.isEmpty() && suggestions.size() < m_suggestCount) {
            suggestions.append(line);
        }
    }

    if (suggestions.isEmpty()) {
        suggestions.append("好的，我明白了");
        suggestions.append("嗯，有道理");
        suggestions.append("那我们试试吧");
    }

    return suggestions;
}
