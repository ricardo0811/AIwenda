#include "chatclient.h"
#include "apiconfig.h"
#include "messagemodel.h"
#include <QNetworkRequest>
#include <QUrl>
#include <QJsonDocument>
#include <QEventLoop>
#include <QDebug>

ChatClient::ChatClient(QObject *parent)
    : QObject(parent)
    , m_manager(new QNetworkAccessManager(this))
    , m_currentReply(nullptr)
    , m_isLoading(false)
{
}

ChatClient::~ChatClient()
{
    cancelRequest();
}

void ChatClient::sendMessage(const QString &message, const QList<ChatMessage> &history)
{
    if (m_isLoading) {
        emit errorOccurred("已有请求正在进行中");
        return;
    }

    ApiConfig &config = ApiConfig::instance();

    QJsonObject body = buildRequestBody(message, history, false);
    QUrl url(config.apiEndpoint());

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey()).toUtf8());

    setLoading(true);
    m_currentReply = m_manager->post(request, QJsonDocument(body).toJson());

    connect(m_currentReply, &QNetworkReply::finished, this, &ChatClient::onFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &ChatClient::onError);
}

void ChatClient::sendStreamMessage(const QString &message, const QList<ChatMessage> &history)
{
    if (m_isLoading) {
        emit errorOccurred("已有请求正在进行中");
        return;
    }

    ApiConfig &config = ApiConfig::instance();

    QJsonObject body = buildRequestBody(message, history, true);
    QUrl url(config.apiEndpoint());

    // 调试日志
    qDebug() << "API端点:" << url.toString();
    qDebug() << "API密钥:" << config.apiKey().left(10) + "...";
    qDebug() << "模型:" << config.model();
    qDebug() << "请求体:" << QJsonDocument(body).toJson(QJsonDocument::Compact);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(config.apiKey()).toUtf8());
    request.setRawHeader("Accept", "text/event-stream");

    setLoading(true);
    m_currentStreamContent.clear();
    m_sseBuffer.clear();
    m_currentReply = m_manager->post(request, QJsonDocument(body).toJson());

    connect(m_currentReply, &QNetworkReply::readyRead, this, &ChatClient::onReadyRead);
    connect(m_currentReply, &QNetworkReply::finished, this, &ChatClient::onFinished);
    connect(m_currentReply, &QNetworkReply::errorOccurred, this, &ChatClient::onError);
}

void ChatClient::cancelRequest()
{
    if (m_currentReply) {
        m_currentReply->abort();
        m_currentReply->deleteLater();
        m_currentReply = nullptr;
        setLoading(false);
    }
}

bool ChatClient::isLoading() const
{
    return m_isLoading;
}

void ChatClient::onReadyRead()
{
    if (!m_currentReply) return;

    QByteArray newData = m_currentReply->readAll();
    m_sseBuffer.append(newData);
    
    qDebug() << "收到数据块:" << newData.size() << "字节, 缓冲区总大小:" << m_sseBuffer.size();
    
    // 立即尝试解析所有可用数据
    parseSSEBuffer();
}

void ChatClient::onFinished()
{
    if (!m_currentReply) {
        setLoading(false);
        return;
    }

    if (m_currentReply->error() == QNetworkReply::NoError) {
        qDebug() << "请求完成 - 已接收内容长度:" << m_currentStreamContent.size();
        
        // 处理剩余的缓冲区数据（包括不完整的最后一个事件）
        if (!m_sseBuffer.isEmpty()) {
            qDebug() << "处理剩余缓冲区数据:" << m_sseBuffer.size();
            
            // 先尝试正常解析
            parseSSEBuffer();
            
            // 如果还有剩余数据（可能是最后一个不完整的事件），直接尝试解析
            if (!m_sseBuffer.isEmpty()) {
                QString remaining = QString::fromUtf8(m_sseBuffer).trimmed();
                if (remaining.startsWith("data:")) {
                    QString dataPart = remaining.mid(5).trimmed();
                    qDebug() << "处理最后一个不完整事件:" << dataPart.left(100);
                    
                    if (dataPart != "[DONE]" && !dataPart.isEmpty()) {
                        QJsonParseError err;
                        QJsonDocument doc = QJsonDocument::fromJson(dataPart.toUtf8(), &err);
                        if (err.error == QJsonParseError::NoError && doc.isObject()) {
                            QJsonObject obj = doc.object();
                            QJsonArray choices = obj["choices"].toArray();
                            if (!choices.isEmpty()) {
                                QJsonObject choice = choices.first().toObject();
                                QJsonObject delta = choice["delta"].toObject();
                                QString content = delta["content"].toString();
                                if (!content.isEmpty()) {
                                    m_currentStreamContent += content;
                                    emit streamChunkReceived(content);
                                }
                            }
                        }
                    }
                }
                m_sseBuffer.clear();
            }
        }
        
        // 如果有流式内容，发送完成信号
        if (!m_currentStreamContent.isEmpty()) {
            qDebug() << "流式响应完成，总内容长度:" << m_currentStreamContent.size();
            emit streamComplete();
        } else {
            qDebug() << "警告: 未收到任何流式内容";
            emit errorOccurred("AI 没有返回任何内容，请重试");
        }
    } else {
        qDebug() << "请求完成时发生错误:" << m_currentReply->errorString();
    }

    m_currentReply->deleteLater();
    m_currentReply = nullptr;
    setLoading(false);
}

void ChatClient::onError(QNetworkReply::NetworkError error)
{
    if (error == QNetworkReply::NetworkError::OperationCanceledError) {
        emit errorOccurred("请求已取消");
    } else {
        QString errorMsg = m_currentReply ? m_currentReply->errorString() : "未知错误";
        qDebug() << "API错误:" << errorMsg << "错误代码:" << error;
        
        // 读取API返回的错误详情
        QString apiErrorDetail;
        if (m_currentReply && m_currentReply->isReadable()) {
            QByteArray errorBody = m_currentReply->readAll();
            if (!errorBody.isEmpty()) {
                qDebug() << "API错误响应:" << QString::fromUtf8(errorBody).left(300);
                apiErrorDetail = QString::fromUtf8(errorBody);
            }
        }
        
        // 分析错误类型并给出友好提示
        int statusCode = m_currentReply ? m_currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() : 0;
        
        if (statusCode == 403) {
            errorMsg = "API 访问被拒绝 (403)。可能原因：\n"
                       "1. API 密钥无效或已过期\n"
                       "2. 账户未完成实名认证（硅基流动要求认证后才能使用）\n"
                       "3. 账户余额不足\n\n"
                       "建议：请登录 https://cloud.siliconflow.cn 检查账户状态";
            if (!apiErrorDetail.isEmpty()) {
                errorMsg += QString("\n\nAPI详情: %1").arg(apiErrorDetail.left(200));
            }
        } else if (statusCode == 401) {
            errorMsg = "API 密钥无效 (401)。请检查 API 密钥是否正确。";
        } else if (statusCode == 400) {
            errorMsg = "请求格式错误 (400)。";
            if (!apiErrorDetail.isEmpty()) {
                errorMsg += QString("\nAPI详情: %1").arg(apiErrorDetail.left(200));
            }
        } else if (statusCode == 429) {
            errorMsg = "请求过于频繁 (429)，请稍后重试。";
        } else if (statusCode >= 500) {
            errorMsg = QString("服务器错误 (%1)，请稍后重试。").arg(statusCode);
        } else {
            if (!apiErrorDetail.isEmpty()) {
                errorMsg += QString(" | API响应: %1").arg(apiErrorDetail.left(200));
            }
        }
        
        emit errorOccurred(errorMsg);
    }
    setLoading(false);
}

QJsonObject ChatClient::buildRequestBody(const QString &message, const QList<ChatMessage> &history, bool stream)
{
    ApiConfig &config = ApiConfig::instance();

    QJsonObject body;
    body["model"] = config.model();
    body["stream"] = stream;

    QJsonArray messages;

    QJsonObject systemMsg;
    systemMsg["role"] = "system";
    systemMsg["content"] = "你是一个友好、智能的AI聊天助手。请用中文简洁、准确地回答用户的问题。";
    messages.append(systemMsg);

    // 只添加有效的历史消息（用户和助手的非空消息）
    int historyCount = qMin(history.size(), 10);
    int startIdx = history.size() - historyCount;
    if (startIdx < 0) startIdx = 0;

    // 先收集有效的历史消息，避免添加重复的用户消息
    QList<QPair<QString, QString>> validHistory;
    for (int i = startIdx; i < history.size(); ++i) {
        const ChatMessage &msg = history[i];
        if ((msg.role == "user" || msg.role == "assistant") && !msg.content.trimmed().isEmpty()) {
            validHistory.append(qMakePair(msg.role, msg.content));
        }
    }

    // 添加历史消息
    for (const auto &item : validHistory) {
        QJsonObject msgObj;
        msgObj["role"] = item.first;
        msgObj["content"] = item.second;
        messages.append(msgObj);
    }

    // 添加当前用户消息（确保不重复）
    bool lastIsCurrentUserMsg = false;
    if (!validHistory.isEmpty()) {
        const auto &lastItem = validHistory.last();
        if (lastItem.first == "user" && lastItem.second == message) {
            lastIsCurrentUserMsg = true;
        }
    }

    if (!lastIsCurrentUserMsg) {
        QJsonObject userMsg;
        userMsg["role"] = "user";
        userMsg["content"] = message;
        messages.append(userMsg);
    }

    body["messages"] = messages;

    // 调试日志
    qDebug() << "构建请求体 - 历史消息数:" << validHistory.size() << "当前消息重复:" << lastIsCurrentUserMsg;
    qDebug() << "最终消息数组大小:" << messages.size();

    return body;
}

void ChatClient::parseSSEBuffer()
{
    while (!m_sseBuffer.isEmpty()) {
        int sepIdx = m_sseBuffer.indexOf("\n\n");
        int sepIdx2 = m_sseBuffer.indexOf("\r\n\r\n");
        
        // 如果没有找到分隔符，跳出循环
        if (sepIdx == -1 && sepIdx2 == -1) break;
        
        int idx = (sepIdx != -1 && sepIdx2 != -1) ? qMin(sepIdx, sepIdx2) : (sepIdx != -1 ? sepIdx : sepIdx2);
        
        QByteArray eventData = m_sseBuffer.left(idx);
        
        // 跳过分隔符
        if (m_sseBuffer.mid(idx, 2) == "\r\n") {
            m_sseBuffer.remove(0, idx + 4);
        } else {
            m_sseBuffer.remove(0, idx + 2);
        }
        
        // 解析 SSE 事件
        QString eventStr = QString::fromUtf8(eventData).trimmed();
        if (eventStr.isEmpty()) continue;
        
        // 提取 data: 行
        QStringList lines = eventStr.split('\n');
        QStringList dataParts;
        
        for (const QString &line : lines) {
            QString trimmedLine = line.trimmed();
            if (trimmedLine.startsWith("data:")) {
                QString value = trimmedLine.mid(5).trimmed();
                dataParts.append(value);
            }
        }
        
        if (dataParts.isEmpty()) continue;
        
        QString dataContent = dataParts.join("\n");
        qDebug() << "SSE data内容:" << dataContent.left(100);
        
        // 处理 [DONE]
        if (dataContent == "[DONE]") {
            qDebug() << "收到 [DONE] 信号";
            continue;  // 使用 continue 而不是 break，继续处理后面可能的数据
        }
        
        // 解析 JSON
        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(dataContent.toUtf8(), &parseError);
        
        if (parseError.error != QJsonParseError::NoError) {
            qDebug() << "JSON解析错误:" << parseError.errorString() << "数据:" << dataContent.left(80);
            continue;
        }
        
        if (!doc.isObject()) continue;
        
        QJsonObject obj = doc.object();
        
        // 处理流式响应
        QJsonArray choices = obj["choices"].toArray();
        if (!choices.isEmpty()) {
            QJsonObject choice = choices.first().toObject();
            
            // 获取 delta 内容
            QJsonObject delta = choice["delta"].toObject();
            QString content = delta["content"].toString();
            
            // 也检查 reasoning_content
            QString reasoningContent = delta["reasoning_content"].toString();
            if (!reasoningContent.isEmpty()) {
                qDebug() << "思考内容:" << reasoningContent.left(50);
            }
            
            if (!content.isEmpty()) {
                m_currentStreamContent += content;
                emit streamChunkReceived(content);
            }
            
            // 检查 finish_reason
            QString finishReason = choice["finish_reason"].toString();
            if (!finishReason.isEmpty()) {
                qDebug() << "完成原因:" << finishReason;
            }
        }
    }
}

void ChatClient::processStreamData()
{
    parseSSEBuffer();
}

QList<QPair<QString, QString>> ChatClient::parseSSEvents()
{
    return parseSSEventsFromData(m_sseBuffer);
}

QList<QPair<QString, QString>> ChatClient::parseSSEventsFromData(const QByteArray &data)
{
    QList<QPair<QString, QString>> events;
    QString dataStr = QString::fromUtf8(data);
    QStringList lines = dataStr.split('\n');

    QString currentEvent;
    QString currentData;

    for (const QString &line : lines) {
        if (line.startsWith("event:")) {
            currentEvent = line.mid(6).trimmed();
        } else if (line.startsWith("data:")) {
            if (!currentData.isEmpty()) currentData += "\n";
            currentData += line.mid(5).trimmed();
        } else if (line.isEmpty() && (!currentEvent.isEmpty() || !currentData.isEmpty())) {
            events.append(qMakePair(currentEvent, currentData));
            currentEvent.clear();
            currentData.clear();
        }
    }

    if (!currentEvent.isEmpty() || !currentData.isEmpty()) {
        events.append(qMakePair(currentEvent, currentData));
    }

    return events;
}

void ChatClient::setLoading(bool loading)
{
    if (m_isLoading != loading) {
        m_isLoading = loading;
        emit loadingChanged();
    }
}
