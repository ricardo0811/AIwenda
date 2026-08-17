#ifndef APICONFIG_H
#define APICONFIG_H

#include <QObject>
#include <QSettings>
#include <QString>

class ApiConfig : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY apiKeyChanged)
    Q_PROPERTY(QString apiEndpoint READ apiEndpoint WRITE setApiEndpoint NOTIFY apiEndpointChanged)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY modelChanged)

public:
    static ApiConfig& instance();

    QString apiKey() const;
    void setApiKey(const QString &key);

    QString apiEndpoint() const;
    void setApiEndpoint(const QString &endpoint);

    QString model() const;
    void setModel(const QString &model);

    QString translateApiEndpoint() const;
    QString sentimentApiEndpoint() const;

    void loadFromFile(const QString &filePath);
    void saveToFile(const QString &filePath);

signals:
    void apiKeyChanged();
    void apiEndpointChanged();
    void modelChanged();

private:
    explicit ApiConfig(QObject *parent = nullptr);
    ~ApiConfig() override;

    ApiConfig(const ApiConfig&) = delete;
    ApiConfig& operator=(const ApiConfig&) = delete;

    QSettings m_settings;
    QString m_apiKey;
    QString m_apiEndpoint;
    QString m_model;
    QString m_translateEndpoint;
    QString m_sentimentEndpoint;
};

#endif // APICONFIG_H
