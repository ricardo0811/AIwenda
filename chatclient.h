#ifndef CHATCLIENT_H
#define CHATCLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QList>
#include <QPair>

#include "messagemodel.h"

class ChatClient : public QObject
{
    Q_OBJECT

public:
    explicit ChatClient(QObject *parent = nullptr);
    ~ChatClient() override;

    void sendMessage(const QString &message, const QList<ChatMessage> &history = QList<ChatMessage>());
    void sendStreamMessage(const QString &message, const QList<ChatMessage> &history = QList<ChatMessage>());
    void cancelRequest();

    bool isLoading() const;

signals:
    void messageReceived(const QString &response, bool isComplete);
    void streamChunkReceived(const QString &chunk);
    void streamComplete();
    void errorOccurred(const QString &errorMessage);
    void loadingChanged();

private slots:
    void onReadyRead();
    void onFinished();
    void onError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_currentReply;
    bool m_isLoading;
    QString m_currentStreamContent;
    QByteArray m_sseBuffer;

    QJsonObject buildRequestBody(const QString &message, const QList<ChatMessage> &history, bool stream);
    void parseSSEBuffer();
    void processStreamData();
    QList<QPair<QString, QString>> parseSSEvents();
    QList<QPair<QString, QString>> parseSSEventsFromData(const QByteArray &data);

    void setLoading(bool loading);
};

#endif // CHATCLIENT_H
