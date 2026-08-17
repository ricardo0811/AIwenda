#include "translationservice.h"
#include "apiconfig.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

TranslationService::TranslationService(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
{
}

TranslationService::~TranslationService()
{
    cancel();
}

void TranslationService::translate(const QString &text, const QString &sourceLang, const QString &targetLang)
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }

    ApiConfig &config = ApiConfig::instance();

    m_pendingText = text;
    QJsonObject body = buildTranslateRequest(text, sourceLang, targetLang);

    QUrl url(config.translateApiEndpoint());
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey()).toUtf8());

    m_currentReply = m_manager->post(request, QJsonDocument(body).toJson());
    connect(m_currentReply, &QNetworkReply::finished, this, &TranslationService::onReplyFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &TranslationService::onReplyError);
}

void TranslationService::cancel()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
    }
}

void TranslationService::onReplyFinished()
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
                emit translationCompleted(content, m_pendingText);
            }
        }
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

void TranslationService::onReplyError(QNetworkReply::NetworkError error)
{
    if (error != QNetworkReply::NetworkError::OperationCanceledError) {
        emit errorOccurred(m_currentReply ? m_currentReply->errorString() : "翻译请求失败");
    }
    m_currentReply->deleteLater();
    m_currentReply = nullptr;
}

QJsonObject TranslationService::buildTranslateRequest(const QString &text, const QString &sourceLang, const QString &targetLang)
{
    ApiConfig &config = ApiConfig::instance();

    QJsonObject body;
    body["model"] = config.model();

    QJsonArray messages;

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = QString("你是一个专业的翻译助手。请将用户输入的文本从%1翻译成%2。只返回翻译结果，不要添加任何解释。").arg(sourceLang).arg(targetLang);
    messages.append(systemMsg);

    QJsonObject userMsg;
    userMsg["role"] = "user";
    userMsg["content"] = text;
    messages.append(userMsg);

    body["messages"] = messages;

    return body;
}
