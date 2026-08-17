#ifndef SENTIMENTANALYZER_H
#define SENTIMENTANALYZER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

class SentimentAnalyzer : public QObject
{
    Q_OBJECT

public:
    explicit SentimentAnalyzer(QObject *parent = nullptr);
    ~SentimentAnalyzer() override;

    void analyze(const QString &text);
    void cancel();

    static QString sentimentToEmoji(const QString &sentiment);
    static QString sentimentToColor(const QString &sentiment);

signals:
    void analysisCompleted(const QString &sentiment, double confidence, const QString &originalText);
    void errorOccurred(const QString &errorMessage);

private slots:
    void onReplyFinished();
    void onReplyError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_currentReply;
    QString m_pendingText;

    QJsonObject buildAnalysisRequest(const QString &text);
    QString parseSentiment(const QString &response);
};

#endif // SENTIMENTANALYZER_H
