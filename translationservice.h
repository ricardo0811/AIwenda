#ifndef TRANSLATIONSERVICE_H
#define TRANSLATIONSERVICE_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>

class TranslationService : public QObject
{
    Q_OBJECT

public:
    explicit TranslationService(QObject *parent = nullptr);
    ~TranslationService() override;

    void translate(const QString &text, const QString &sourceLang = "auto", const QString &targetLang = "zh");
    void cancel();

signals:
    void translationCompleted(const QString &translatedText, const QString &originalText);
    void errorOccurred(const QString &errorMessage);

private slots:
    void onReplyFinished();
    void onReplyError(QNetworkReply::NetworkError error);

private:
    QNetworkAccessManager *m_manager;
    QNetworkReply *m_currentReply;
    QString m_pendingText;

    QJsonObject buildTranslateRequest(const QString &text, const QString &sourceLang, const QString &targetLang);
};

#endif // TRANSLATIONSERVICE_H
