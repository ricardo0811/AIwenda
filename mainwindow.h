#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QLabel>
#include <QScrollArea>
#include <QSplitter>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QWidget>

#include "chatclient.h"
#include "translationservice.h"
#include "sentimentanalyzer.h"
#include "replysuggester.h"
#include "messagemodel.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    void onSendMessage();
    void onStreamChunkReceived(const QString &chunk);
    void onStreamComplete();
    void onMessageReceived(const QString &response, bool isComplete);
    void onErrorOccurred(const QString &errorMessage);

    void onTranslationCompleted(const QString &translatedText, const QString &originalText);
    void onAnalysisCompleted(const QString &sentiment, double confidence, const QString &originalText);
    void onSuggestionsReady(const QStringList &suggestions, const QString &originalMessage);

    void onClearChat();
    void onRegenerateLastAI();
    void onToggleTranslation(bool checked);

private:
    void setupUI();
    void setupConnections();
    void setupStyles();

    void addUserMessage(const QString &content, const QString &id = QString());
    void addAIMessage(const QString &content, const QString &id = QString());
    void addSystemMessage(const QString &content);

    void createMessageBubble(const ChatMessage &msg);
    void createSuggestionButtons(const QStringList &suggestions);
    void hideSuggestions();
    void updateTypingIndicator(bool isTyping);
    void removeMessageBubble(const QString &id);

    void saveHistory();
    void loadHistory();

    Ui::MainWindow *ui;

    ChatClient *m_chatClient;
    TranslationService *m_translationService;
    SentimentAnalyzer *m_sentimentAnalyzer;
    ReplySuggester *m_replySuggester;
    MessageModel *m_messageModel;

    QString m_currentAIResponseId;
    QString m_pendingUserMessage;
    QLabel *m_currentAIContentLabel;
    QListWidget *m_messageList;
    QLineEdit *m_inputEdit;
    QPushButton *m_sendBtn;
    QLabel *m_statusLabel;
    QLabel *m_typingLabel;
    QScrollArea *m_suggestionArea;
    QWidget *m_suggestionWidget;
    QVBoxLayout *m_suggestionLayout;
    QCheckBox *m_translateCheckbox;

    QSplitter *m_mainSplitter;
    QWidget *m_chatPanel;
    QWidget *m_sidePanel;
    QTextEdit *m_translationDisplay;
    QLabel *m_emotionLabel;

    bool m_showTranslation;
    qint64 m_lastUpdateTime;
};

#endif // MAINWINDOW_H
