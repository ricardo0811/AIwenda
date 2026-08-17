#include "sentimentanalyzer.h"
#include "apiconfig.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QRegularExpression>

SentimentAnalyzer::SentimentAnalyzer(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
{
}

SentimentAnalyzer::~SentimentAnalyzer()
{
    cancel();
}

void SentimentAnalyzer::analyze(const QString &text)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    ApiConfig &config = ApiConfig::instance();

    m_pendingText = text;
    QJsonObject body = buildAnalysisRequest(text);

    QUrl url(config.sentimentApiEndpoint());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey()).toUtf8());

    m_currentReply = m_manager->post(request, QJsonDocument(body).toJson());
    connect(m_currentReply, &QNetworkReply::finished, this, &SentimentAnalyzer::onReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &SentimentAnalyzer::onReplyError);
}

void SentimentAnalyzer::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void SentimentAnalyzer::onReplyFinished()
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
                QString sentiment = parseSentiment(content);
                emit analysisCompleted(sentiment, 0.8, m_pendingText);
            }
        }
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void SentimentAnalyzer::onReplyError(QNetworkReply::NetworkError error)
{
    if (error != QNetworkReply::NetworkError::OperationCanceledError) {
        emit errorOccurred(m_currentReply ? m_currentReply->errorString() : "情感分析请求失败");
    }
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

QString SentimentAnalyzer::sentimentToEmoji(const QString &sentiment)
{
    if (sentiment == "positive") return "😊";
    if (sentiment == "negative") return "😟";
    return "😐";
}

QString SentimentAnalyzer::sentimentToColor(const QString &sentiment)
{
    if (sentiment == "positive") return "#4CAF50";
    if (sentiment == "negative") return "#f44336";
    return "#9E9E9E";
}

QJsonObject SentimentAnalyzer::buildAnalysisRequest(const QString &text)
{
    ApiConfig &config = ApiConfig::instance();

    QJsonObject body;
    body["model"] = config.model();

    QJsonArray messages;

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "你是一个情感分析专家。请分析用户输入文本的情感倾向，只能返回以下三个标签之一：positive（积极）、negative（消极）、neutral（中性）。只返回标签，不要添加任何解释。";
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = text;
    messages.append(userMsg);

    body["messages"] = messages;

    return body;
}

QString SentimentAnalyzer::parseSentiment(const QString &response)
{
    QString lower = response.toLower().trimmed();

    if (lower.contains("positive") || lower.contains("积极") || lower.contains("正面")) {
        return "positive";
    }
    if (lower.contains("negative") || lower.contains("消极") || lower.contains("负面") || lower.contains("negative")) {
        return "negative";
    }
    if (lower.contains("neutral") || lower.contains("中性") || lower.contains("中立")) {
        return "neutral";
    }

    if (lower.contains("😊") || lower.contains("happy") || lower.contains("good")) {
        return "positive";
    }
    if (lower.contains("😟") || lower.contains("sad") || lower.contains("bad")) {
        return "negative";
    }

    return "neutral";
}
