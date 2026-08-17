#include "apiconfig.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

ApiConfig& ApiConfig::instance()
{
    static ApiConfig instance;
    return instance;
}

ApiConfig::ApiConfig(QObject *parent)
    : QObject(parent)
    , m_settings(QCoreApplication::applicationDirPath() + "/config.ini", QSettings::IniFormat)
    , m_apiKey("YOUR_API_KEY_HERE")  // 请在 config.ini 中配置真实密钥,切勿硬编码提交
    , m_apiEndpoint("https://api.siliconflow.cn/v1/chat/completions")
    , m_model("deepseek-ai/DeepSeek-V3")
    , m_translateEndpoint("https://api.siliconflow.cn/v1/chat/completions")
    , m_sentimentEndpoint("https://api.siliconflow.cn/v1/chat/completions")
{
    // 在多个位置查找 config.ini
    QStringList configPaths;
    QString appDir = QCoreApplication::applicationDirPath();
    
    // 1. 可执行文件所在目录
    configPaths << (appDir + "/config.ini");
    // 2. 上一级目录（build/debug/ -> build/）
    configPaths << (appDir + "/../config.ini");
    // 3. 上两级目录（build/ -> 项目根目录）
    configPaths << (appDir + "/../../config.ini");
    // 4. 上三级目录
    configPaths << (appDir + "/../../../config.ini");
    
    QString foundConfig;
    for (const QString &path : configPaths) {
        QString normPath = QDir::cleanPath(path);
        if (QFile::exists(normPath)) {
            foundConfig = normPath;
            qDebug() << "找到配置文件:" << normPath;
            break;
        }
    }
    
    if (!foundConfig.isEmpty()) {
        QSettings settings(foundConfig, QSettings::IniFormat);
        
        QString fileKey = settings.value("api/key", "").toString().trimmed();
        if (!fileKey.isEmpty() && fileKey.startsWith("sk-")) {
            m_apiKey = fileKey;
        }
        
        QString fileEndpoint = settings.value("api/endpoint", "").toString().trimmed();
        if (!fileEndpoint.isEmpty() && fileEndpoint.startsWith("http")) {
            m_apiEndpoint = fileEndpoint;
        }
        
        QString fileModel = settings.value("api/model", "").toString().trimmed();
        if (!fileModel.isEmpty()) {
            m_model = fileModel;
        }
        
        qDebug() << "已从" << foundConfig << "加载配置";
    } else {
        qDebug() << "未找到config.ini，使用硬编码配置";
    }
    
    qDebug() << "API配置加载完成 - 端点:" << m_apiEndpoint << "模型:" << m_model 
             << "密钥前缀:" << m_apiKey.left(10) << "...";
}

ApiConfig::~ApiConfig()
{
}

QString ApiConfig::apiKey() const
{
    return m_apiKey;
}

void ApiConfig::setApiKey(const QString &key)
{
    if (m_apiKey != key) {
        m_apiKey = key;
        m_settings.setValue("api/key", key);
        emit apiKeyChanged();
    }
}

QString ApiConfig::apiEndpoint() const
{
    return m_apiEndpoint;
}

void ApiConfig::setApiEndpoint(const QString &endpoint)
{
    if (m_apiEndpoint != endpoint) {
        m_apiEndpoint = endpoint;
        m_settings.setValue("api/endpoint", endpoint);
        emit apiEndpointChanged();
    }
}

QString ApiConfig::model() const
{
    return m_model;
}

void ApiConfig::setModel(const QString &model)
{
    if (m_model != model) {
        m_model = model;
        m_settings.setValue("api/model", model);
        emit modelChanged();
    }
}

QString ApiConfig::translateApiEndpoint() const
{
    return m_translateEndpoint;
}

QString ApiConfig::sentimentApiEndpoint() const
{
    return m_sentimentEndpoint;
}

void ApiConfig::loadFromFile(const QString &filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains("api_key")) setApiKey(obj["api_key"].toString());
        if (obj.contains("api_endpoint")) setApiEndpoint(obj["api_endpoint"].toString());
        if (obj.contains("model")) setModel(obj["model"].toString());
    }
}

void ApiConfig::saveToFile(const QString &filePath)
{
    QJsonObject obj;
    obj["api_key"] = m_apiKey;
    obj["api_endpoint"] = m_apiEndpoint;
    obj["model"] = m_model;

    QJsonDocument doc(obj);
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
    }
}
