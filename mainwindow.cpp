#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QListWidgetItem>
#include <QMessageBox>
#include <QDateTime>
#include <QScrollBar>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QInputDialog>
#include <QNetworkInterface>
#include <QUuid>
#include <QTimer>
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_chatClient(new ChatClient(this))
    , m_translationService(new TranslationService(this))
    , m_sentimentAnalyzer(new SentimentAnalyzer(this))
    , m_replySuggester(new ReplySuggester(this))
    , m_messageModel(new MessageModel(this))
    , m_currentAIContentLabel(nullptr)
    , m_showTranslation(true)
    , m_lastUpdateTime(0)
{
    ui->setupUi(this);
    setupUI();
    setupConnections();
    setupStyles();

    setWindowTitle("AI智能聊天助手 - AIwenda");
    resize(1200, 800);

    loadHistory();
}

MainWindow::~MainWindow()
{
    saveHistory();
    delete ui;
}

void MainWindow::setupUI()
{
    m_mainSplitter = new QSplitter(Qt::Horizontal, this);
    setCentralWidget(m_mainSplitter);

    m_chatPanel = new QWidget(this);
    QVBoxLayout *chatLayout = new QVBoxLayout(m_chatPanel);
    chatLayout->setContentsMargins(8, 8, 8, 8);
    chatLayout->setSpacing(6);

    QHBoxLayout *topBar = new QHBoxLayout();
    
    m_translateCheckbox = new QCheckBox("实时翻译", this);
    m_translateCheckbox->setChecked(true);
    QPushButton *clearBtn = new QPushButton("清空对话", this);
    clearBtn->setObjectName("clearBtn");
    QPushButton *regenBtn = new QPushButton("重新生成", this);
    regenBtn->setObjectName("regenBtn");

    topBar->addWidget(m_translateCheckbox);
    topBar->addStretch();
    topBar->addWidget(regenBtn);
    topBar->addWidget(clearBtn);
    chatLayout->addLayout(topBar);

    m_messageList = new QListWidget(this);
    m_messageList->setStyleSheet(
        "QListWidget { border: none; background-color: #f5f5f5; outline: 0; }"
        "QListWidget::item { padding: 0; margin: 0; }"
        "QScrollBar:vertical { width: 6px; background: transparent; }"
        "QScrollBar::handle:vertical { background: rgba(0,0,0,0.2); border-radius: 3px; min-height: 30px; }"
        "QScrollBar::handle:vertical:hover { background: rgba(0,0,0,0.3); }"
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }"
    );
    m_messageList->setSelectionMode(QAbstractItemView::NoSelection);
    m_messageList->setSpacing(0);
    m_messageList->setUniformItemSizes(false);
    m_messageList->setWordWrap(true);
    chatLayout->addWidget(m_messageList, 1);

    m_suggestionArea = new QScrollArea(this);
    m_suggestionArea->setWidgetResizable(true);
    m_suggestionArea->setMaximumHeight(60);
    m_suggestionArea->setStyleSheet("QScrollArea { border: none; background: transparent; }");
    m_suggestionWidget = new QWidget();
    m_suggestionLayout = new QVBoxLayout(m_suggestionWidget);
    m_suggestionLayout->setContentsMargins(4, 2, 4, 2);
    m_suggestionLayout->setSpacing(4);
    m_suggestionLayout->addStretch();
    m_suggestionArea->setWidget(m_suggestionWidget);
    m_suggestionArea->hide();
    chatLayout->addWidget(m_suggestionArea);

    m_typingLabel = new QLabel(this);
    m_typingLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_typingLabel->setStyleSheet("color: #888; font-style: italic; padding: 2px;");
    m_typingLabel->hide();
    chatLayout->addWidget(m_typingLabel);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    m_inputEdit = new QLineEdit(this);
    m_inputEdit->setPlaceholderText("输入消息... (Enter发送)");
    m_inputEdit->setStyleSheet(
        "QLineEdit { padding: 10px; border: 2px solid #ddd; border-radius: 8px; font-size: 14px; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
    );
    m_sendBtn = new QPushButton("发送", this);
    m_sendBtn->setFixedWidth(80);
    m_sendBtn->setStyleSheet(
        "QPushButton { background-color: #4CAF50; color: white; border: none; "
        "border-radius: 8px; padding: 10px 20px; font-size: 14px; font-weight: bold; }"
        "QPushButton:hover { background-color: #45a049; }"
        "QPushButton:disabled { background-color: #ccc; }"
    );
    inputLayout->addWidget(m_inputEdit, 1);
    inputLayout->addWidget(m_sendBtn);
    chatLayout->addLayout(inputLayout);

    m_mainSplitter->addWidget(m_chatPanel);

    m_sidePanel = new QWidget(this);
    QVBoxLayout *sideLayout = new QVBoxLayout(m_sidePanel);
    sideLayout->setContentsMargins(8, 8, 8, 8);
    sideLayout->setSpacing(8);

    QLabel *translateTitle = new QLabel("实时翻译", this);
    translateTitle->setStyleSheet("font-weight: bold; font-size: 14px; color: #333;");
    sideLayout->addWidget(translateTitle);

    m_translationDisplay = new QTextEdit(this);
    m_translationDisplay->setReadOnly(true);
    m_translationDisplay->setPlaceholderText("翻译结果将显示在这里...");
    m_translationDisplay->setStyleSheet(
        "QTextEdit { border: 1px solid #ddd; border-radius: 8px; padding: 8px; "
        "background-color: #fafafa; font-size: 13px; }"
    );
    sideLayout->addWidget(m_translationDisplay, 1);

    QLabel *emotionTitle = new QLabel("情感状态", this);
    emotionTitle->setStyleSheet("font-weight: bold; font-size: 14px; color: #333; margin-top: 10px;");
    sideLayout->addWidget(emotionTitle);

    m_emotionLabel = new QLabel("😊 中性", this);
    m_emotionLabel->setAlignment(Qt::AlignCenter);
    m_emotionLabel->setStyleSheet(
        "font-size: 18px; padding: 15px; border: 2px solid #ddd; border-radius: 10px; "
        "background-color: #fff;"
    );
    sideLayout->addWidget(m_emotionLabel);

    m_mainSplitter->addWidget(m_sidePanel);
    m_mainSplitter->setStretchFactor(0, 3);
    m_mainSplitter->setStretchFactor(1, 1);
    m_mainSplitter->setSizes(QList<int>() << 800 << 300);

    statusBar()->showMessage("就绪");
    m_statusLabel = new QLabel("AI聊天助手 v1.0");
    statusBar()->addPermanentWidget(m_statusLabel);
}

void MainWindow::setupConnections()
{
    connect(m_sendBtn, &QPushButton::clicked, this, &MainWindow::onSendMessage);
    connect(m_inputEdit, &QLineEdit::returnPressed, this, &MainWindow::onSendMessage);

    connect(m_chatClient, &ChatClient::streamChunkReceived,
            this, &MainWindow::onStreamChunkReceived);
    connect(m_chatClient, &ChatClient::streamComplete,
            this, &MainWindow::onStreamComplete);
    connect(m_chatClient, &ChatClient::messageReceived,
            this, &MainWindow::onMessageReceived);
    connect(m_chatClient, &ChatClient::errorOccurred,
            this, &MainWindow::onErrorOccurred);
    connect(m_chatClient, &ChatClient::loadingChanged, this, [this]() {
        m_sendBtn->setEnabled(!m_chatClient->isLoading());
        m_inputEdit->setEnabled(!m_chatClient->isLoading());
    });

    connect(m_translationService, &TranslationService::translationCompleted,
            this, &MainWindow::onTranslationCompleted);
    connect(m_translationService, &TranslationService::errorOccurred,
            this, [this](const QString &err) {
                qWarning() << "Translation error:" << err;
            });

    connect(m_sentimentAnalyzer, &SentimentAnalyzer::analysisCompleted,
            this, &MainWindow::onAnalysisCompleted);

    connect(m_replySuggester, &ReplySuggester::suggestionsReady,
            this, &MainWindow::onSuggestionsReady);

    QPushButton *clearBtn = findChild<QPushButton*>("clearBtn");
    if (clearBtn) connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClearChat);

    QPushButton *regenBtn = findChild<QPushButton*>("regenBtn");
    if (regenBtn) connect(regenBtn, &QPushButton::clicked, this, &MainWindow::onRegenerateLastAI);

    connect(m_translateCheckbox, &QCheckBox::toggled,
            this, &MainWindow::onToggleTranslation);
}

void MainWindow::setupStyles()
{
    setStyleSheet(
        "QMainWindow { background-color: #f0f2f5; }"
        "QPushButton { cursor: pointer; border: none; border-radius: 6px; padding: 6px 16px; font-size: 13px; }"
        "QPushButton:disabled { cursor: not-allowed; opacity: 0.6; }"
        "QLabel { font-family: 'Microsoft YaHei', 'PingFang SC', sans-serif; }"
        "QLineEdit { border: 1px solid #ddd; border-radius: 8px; padding: 8px 12px; font-size: 14px; background: white; }"
        "QLineEdit:focus { border-color: #4CAF50; }"
        "QCheckBox { font-size: 12px; color: #666; }"
        "QScrollArea { border: none; background: transparent; }"
    );
}

void MainWindow::onSendMessage()
{
    QString text = m_inputEdit->text().trimmed();
    if (text.isEmpty()) return;

    addUserMessage(text);
    m_pendingUserMessage = text;
    m_inputEdit->clear();

    ChatMessage userMsg;
    userMsg.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    userMsg.role = "user";
    userMsg.content = text;
    userMsg.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    userMsg.isAI = false;
    userMsg.senderName = "我";
    m_messageModel->addMessage(userMsg);

    // 在添加placeholder之前获取历史，避免包含空内容消息
    QList<ChatMessage> history = m_messageModel->recentMessages(10);

    m_currentAIResponseId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    ChatMessage placeholderMsg;
    placeholderMsg.id = m_currentAIResponseId;
    placeholderMsg.role = "assistant";
    placeholderMsg.content = "";
    placeholderMsg.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    placeholderMsg.isAI = true;
    placeholderMsg.senderName = "AI助手";
    m_messageModel->addMessage(placeholderMsg);

    createMessageBubble(placeholderMsg);

    updateTypingIndicator(true);

    m_chatClient->sendStreamMessage(text, history);

    if (m_showTranslation) {
        m_translationService->translate(text);
    }

    m_sentimentAnalyzer->analyze(text);

    QTimer::singleShot(1000, this, [this]() {
        if (!m_chatClient->isLoading()) return;
        m_replySuggester->suggest(m_pendingUserMessage);
    });

    saveHistory();
}

void MainWindow::onStreamChunkReceived(const QString &chunk)
{
    if (m_currentAIResponseId.isEmpty()) return;

    ChatMessage currentMsg = m_messageModel->getMessage(m_currentAIResponseId);
    QString newContent = currentMsg.content + chunk;
    m_messageModel->updateMessageContent(m_currentAIResponseId, newContent);

    // 节流：每 50ms 才更新一次 UI
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if (now - m_lastUpdateTime < 50) {
        // 只更新数据，不刷新 UI
        return;
    }
    m_lastUpdateTime = now;

    // 更新 label 文本
    if (m_currentAIContentLabel) {
        m_currentAIContentLabel->setText(newContent);
        
        // 调整气泡大小
        QWidget *bubble = m_currentAIContentLabel->parentWidget();
        if (bubble) {
            bubble->adjustSize();
            
            // 找到对应的 QListWidgetItem 并更新大小
            for (int i = 0; i < m_messageList->count(); ++i) {
                QListWidgetItem *item = m_messageList->item(i);
                if (m_messageList->itemWidget(item) == bubble) {
                    item->setSizeHint(QSize(m_messageList->viewport()->width(), bubble->sizeHint().height()));
                    m_messageList->doItemsLayout();
                    break;
                }
            }
        }
    }

    m_messageList->verticalScrollBar()->setValue(m_messageList->verticalScrollBar()->maximum());
}

void MainWindow::onStreamComplete()
{
    updateTypingIndicator(false);

    if (m_currentAIResponseId.isEmpty()) return;

    ChatMessage lastMsg = m_messageModel->getMessage(m_currentAIResponseId);
    if (!lastMsg.isAI || lastMsg.content.isEmpty()) {
        removeMessageBubble(m_currentAIResponseId);
        m_messageModel->removeMessage(m_currentAIResponseId);
        m_currentAIContentLabel = nullptr;
        m_currentAIResponseId.clear();
        return;
    }

    // 强制最后一次 UI 更新 - 确保使用 model 中的最新内容
    QLabel *targetLabel = m_currentAIContentLabel;
    
    // 如果直接引用丢失，通过遍历列表查找
    if (!targetLabel) {
        for (int i = 0; i < m_messageList->count(); ++i) {
            QListWidgetItem *item = m_messageList->item(i);
            if (item->data(Qt::UserRole).toString() == m_currentAIResponseId) {
                QWidget *widget = m_messageList->itemWidget(item);
                if (widget) {
                    targetLabel = widget->findChild<QLabel*>("aiContentLabel");
                    if (targetLabel) {
                        m_currentAIContentLabel = targetLabel;
                    }
                }
                break;
            }
        }
    }
    
    if (targetLabel) {
        targetLabel->setText(lastMsg.content);
        
        // 最终调整气泡大小
        QWidget *bubble = targetLabel->parentWidget();
        if (bubble) {
            bubble->adjustSize();
            
            for (int i = 0; i < m_messageList->count(); ++i) {
                QListWidgetItem *item = m_messageList->item(i);
                if (m_messageList->itemWidget(item) == bubble) {
                    item->setSizeHint(QSize(m_messageList->viewport()->width(), bubble->sizeHint().height()));
                    m_messageList->doItemsLayout();
                    break;
                }
            }
        }
    }

    m_messageList->verticalScrollBar()->setValue(m_messageList->verticalScrollBar()->maximum());

    m_sentimentAnalyzer->analyze(lastMsg.content);
    m_currentAIContentLabel = nullptr;
    m_currentAIResponseId.clear();
    saveHistory();
}

void MainWindow::onMessageReceived(const QString &response, bool isComplete)
{
    Q_UNUSED(isComplete)
    if (m_currentAIResponseId.isEmpty()) return;
    m_messageModel->updateMessageContent(m_currentAIResponseId, response);

    if (m_currentAIContentLabel) {
        m_currentAIContentLabel->setText(response);
    }
}

void MainWindow::onErrorOccurred(const QString &errorMessage)
{
    updateTypingIndicator(false);

    // 清理失败的AI回复placeholder
    if (!m_currentAIResponseId.isEmpty()) {
        removeMessageBubble(m_currentAIResponseId);
        m_messageModel->removeMessage(m_currentAIResponseId);
        m_currentAIContentLabel = nullptr;
        m_currentAIResponseId.clear();
    }

    statusBar()->showMessage("错误: " + errorMessage, 5000);
    QMessageBox::warning(this, "错误", errorMessage);
}

void MainWindow::onTranslationCompleted(const QString &translatedText, const QString &originalText)
{
    if (!m_showTranslation) return;
    m_translationDisplay->setText(QString("<b>原文:</b> %1<br><br><b>译文:</b> %2").arg(originalText).arg(translatedText));
}

void MainWindow::onAnalysisCompleted(const QString &sentiment, double confidence, const QString &originalText)
{
    Q_UNUSED(confidence)
    Q_UNUSED(originalText)

    QString emoji = SentimentAnalyzer::sentimentToEmoji(sentiment);
    QString color = SentimentAnalyzer::sentimentToColor(sentiment);
    QString label;

    if (sentiment == "positive") label = "积极";
    else if (sentiment == "negative") label = "消极";
    else label = "中性";

    m_emotionLabel->setText(QString("%1 %2").arg(emoji).arg(label));
    m_emotionLabel->setStyleSheet(
        QString("font-size: 18px; padding: 15px; border: 2px solid %1; border-radius: 10px; background-color: #fff;")
        .arg(color)
    );

    if (!m_currentAIResponseId.isEmpty()) {
        m_messageModel->updateMessageSentiment(m_currentAIResponseId, sentiment);
    }
}

void MainWindow::onSuggestionsReady(const QStringList &suggestions, const QString &originalMessage)
{
    Q_UNUSED(originalMessage)
    createSuggestionButtons(suggestions);
}

void MainWindow::onClearChat()
{
    QMessageBox::StandardButton reply = QMessageBox::question(this, "确认",
        "确定要清空所有聊天记录吗？",
        QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        m_messageModel->clear();
        m_messageList->clear();
        hideSuggestions();
        saveHistory();
    }
}

void MainWindow::onRegenerateLastAI()
{
    if (m_chatClient->isLoading()) return;

    QList<ChatMessage> msgs = m_messageModel->messages();
    QString lastUserMsg;
    for (int i = msgs.size() - 1; i >= 0; --i) {
        if (!msgs[i].isAI && msgs[i].role == "user") {
            lastUserMsg = msgs[i].content;
            break;
        }
    }

    if (lastUserMsg.isEmpty()) {
        QMessageBox::information(this, "提示", "没有可重新生成的消息");
        return;
    }

    // 获取历史（排除AI消息的空内容）
    QList<ChatMessage> history = m_messageModel->recentMessages(10);

    m_currentAIResponseId = QUuid::createUuid().toString(QUuid::WithoutBraces);

    ChatMessage placeholderMsg;
    placeholderMsg.id = m_currentAIResponseId;
    placeholderMsg.role = "assistant";
    placeholderMsg.content = "";
    placeholderMsg.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    placeholderMsg.isAI = true;
    placeholderMsg.senderName = "AI助手";
    m_messageModel->addMessage(placeholderMsg);

    createMessageBubble(placeholderMsg);

    updateTypingIndicator(true);
    m_chatClient->sendStreamMessage(lastUserMsg, history);
}

void MainWindow::onToggleTranslation(bool checked)
{
    m_showTranslation = checked;
    m_translationDisplay->setVisible(checked);
    if (!checked) {
        m_translationDisplay->clear();
    }
}

void MainWindow::addUserMessage(const QString &content, const QString &id)
{
    QWidget *bubble = new QWidget();
    bubble->setObjectName("userBubble");
    QHBoxLayout *layout = new QHBoxLayout(bubble);
    layout->setContentsMargins(10, 8, 10, 4);
    layout->setSpacing(8);

    QLabel *avatarLabel = new QLabel("我", bubble);
    avatarLabel->setFixedSize(QSize(32, 32));
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4CAF50, stop:1 #2E7D32);"
        "color: white; border-radius: 16px; font-weight: bold; font-size: 13px;"
    );

    QVBoxLayout *rightLayout = new QVBoxLayout();
    rightLayout->setSpacing(3);
    
    QLabel *nameLabel = new QLabel("我", bubble);
    nameLabel->setAlignment(Qt::AlignRight);
    nameLabel->setStyleSheet("color: #666; font-size: 11px;");

    QLabel *contentLabel = new QLabel(content, bubble);
    contentLabel->setWordWrap(true);
    contentLabel->setAlignment(Qt::AlignRight);
    contentLabel->setStyleSheet(
        "background-color: #4CAF50; color: white; padding: 10px 14px; "
        "border-radius: 12px; font-size: 14px; line-height: 1.4;"
    );
    contentLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    contentLabel->setMaximumWidth(450);

    QLabel *timeLabel = new QLabel(QDateTime::currentDateTime().toString("HH:mm"), bubble);
    timeLabel->setAlignment(Qt::AlignRight);
    timeLabel->setStyleSheet("color: #999; font-size: 10px;");

    rightLayout->addWidget(nameLabel);
    rightLayout->addWidget(contentLabel);
    rightLayout->addWidget(timeLabel);

    layout->addStretch();
    layout->addLayout(rightLayout);
    layout->addWidget(avatarLabel);

    QListWidgetItem *item = new QListWidgetItem(m_messageList);
    item->setData(Qt::UserRole, id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id);
    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, bubble);
    
    // 延迟设置大小，等待布局完成
    QTimer::singleShot(0, this, [this, item, bubble]() {
        bubble->adjustSize();
        item->setSizeHint(QSize(m_messageList->viewport()->width(), bubble->sizeHint().height()));
        m_messageList->doItemsLayout();
    });
    
    m_messageList->verticalScrollBar()->setValue(m_messageList->verticalScrollBar()->maximum());
}

void MainWindow::addAIMessage(const QString &content, const QString &id)
{
    QWidget *bubble = new QWidget();
    bubble->setObjectName("aiBubble");
    QHBoxLayout *layout = new QHBoxLayout(bubble);
    layout->setContentsMargins(10, 8, 10, 4);
    layout->setSpacing(8);

    QLabel *avatarLabel = new QLabel("AI", bubble);
    avatarLabel->setFixedSize(QSize(32, 32));
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2196F3, stop:1 #1565C0);"
        "color: white; border-radius: 16px; font-weight: bold; font-size: 12px;"
    );

    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(3);
    
    QLabel *nameLabel = new QLabel("AI助手", bubble);
    nameLabel->setStyleSheet("color: #666; font-size: 11px;");

    QLabel *contentLabel = new QLabel(content, bubble);
    contentLabel->setWordWrap(true);
    contentLabel->setStyleSheet(
        "background-color: white; color: #333; padding: 10px 14px; "
        "border-radius: 12px; font-size: 14px; line-height: 1.4; "
        "border: 1px solid #e8e8e8;"
    );
    contentLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    contentLabel->setMaximumWidth(450);

    QLabel *timeLabel = new QLabel(QDateTime::currentDateTime().toString("HH:mm"), bubble);
    timeLabel->setStyleSheet("color: #999; font-size: 10px;");

    leftLayout->addWidget(nameLabel);
    leftLayout->addWidget(contentLabel);
    leftLayout->addWidget(timeLabel);

    layout->addWidget(avatarLabel);
    layout->addLayout(leftLayout);
    layout->addStretch();

    QListWidgetItem *item = new QListWidgetItem(m_messageList);
    item->setData(Qt::UserRole, id.isEmpty() ? QUuid::createUuid().toString(QUuid::WithoutBraces) : id);
    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, bubble);
    
    QTimer::singleShot(0, this, [this, item, bubble]() {
        bubble->adjustSize();
        item->setSizeHint(QSize(m_messageList->viewport()->width(), bubble->sizeHint().height()));
        m_messageList->doItemsLayout();
    });
    
    m_messageList->verticalScrollBar()->setValue(m_messageList->verticalScrollBar()->maximum());
}

void MainWindow::addSystemMessage(const QString &content)
{
    QListWidgetItem *item = new QListWidgetItem(m_messageList);
    item->setFlags(item->flags() & ~Qt::ItemIsSelectable);

    QWidget *widget = new QWidget();
    QHBoxLayout *layout = new QHBoxLayout(widget);
    layout->setContentsMargins(10, 5, 10, 5);

    QLabel *label = new QLabel(content, widget);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet(
        "color: #888; font-size: 12px; font-style: italic; "
        "background-color: #e0e0e0; padding: 4px 12px; border-radius: 10px;"
    );

    layout->addStretch();
    layout->addWidget(label);
    layout->addStretch();

    item->setSizeHint(QSize(500, 25));
    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, widget);
}

void MainWindow::createMessageBubble(const ChatMessage &msg)
{
    QWidget *bubble = new QWidget();
    bubble->setObjectName("aiBubble");
    QHBoxLayout *layout = new QHBoxLayout(bubble);
    layout->setContentsMargins(10, 8, 10, 4);
    layout->setSpacing(8);

    QLabel *avatarLabel = new QLabel("AI", bubble);
    avatarLabel->setFixedSize(QSize(32, 32));
    avatarLabel->setAlignment(Qt::AlignCenter);
    avatarLabel->setStyleSheet(
        "background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #2196F3, stop:1 #1565C0);"
        "color: white; border-radius: 16px; font-weight: bold; font-size: 12px;"
    );

    QVBoxLayout *leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(3);
    
    QLabel *nameLabel = new QLabel(msg.senderName.isEmpty() ? "AI助手" : msg.senderName, bubble);
    nameLabel->setStyleSheet("color: #666; font-size: 11px;");

    QLabel *contentLabel = new QLabel(msg.content, bubble);
    contentLabel->setObjectName("aiContentLabel");
    contentLabel->setWordWrap(true);
    contentLabel->setStyleSheet(
        "background-color: white; color: #333; padding: 10px 14px; "
        "border-radius: 12px; font-size: 14px; line-height: 1.4; "
        "border: 1px solid #e8e8e8;"
    );
    contentLabel->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Maximum);
    contentLabel->setMaximumWidth(450);

    if (msg.id == m_currentAIResponseId) {
        m_currentAIContentLabel = contentLabel;
    }

    QLabel *timeLabel = new QLabel(
        msg.timestamp.isEmpty() ? QDateTime::currentDateTime().toString("HH:mm") : msg.timestamp,
        bubble);
    timeLabel->setStyleSheet("color: #999; font-size: 10px;");

    leftLayout->addWidget(nameLabel);
    leftLayout->addWidget(contentLabel);
    leftLayout->addWidget(timeLabel);

    layout->addWidget(avatarLabel);
    layout->addLayout(leftLayout);
    layout->addStretch();

    QListWidgetItem *item = new QListWidgetItem(m_messageList);
    item->setData(Qt::UserRole, msg.id);
    m_messageList->addItem(item);
    m_messageList->setItemWidget(item, bubble);
    
    QTimer::singleShot(0, this, [this, item, bubble]() {
        bubble->adjustSize();
        item->setSizeHint(QSize(m_messageList->viewport()->width(), bubble->sizeHint().height()));
        m_messageList->doItemsLayout();
    });
    
    m_messageList->verticalScrollBar()->setValue(m_messageList->verticalScrollBar()->maximum());
}

void MainWindow::createSuggestionButtons(const QStringList &suggestions)
{
    QLayoutItem *item;
    while ((item = m_suggestionLayout->takeAt(0)) != nullptr) {
        if (item->widget()) delete item->widget();
        delete item;
    }

    QLabel *titleLabel = new QLabel("💡 快速回复建议:", m_suggestionWidget);
    titleLabel->setStyleSheet("color: #666; font-size: 12px; margin-right: 8px;");
    m_suggestionLayout->addWidget(titleLabel);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(8);

    for (const QString &suggestion : suggestions) {
        QPushButton *btn = new QPushButton(suggestion, m_suggestionWidget);
        btn->setStyleSheet(
            "QPushButton { background-color: #E3F2FD; color: #1976D2; border: 1px solid #90CAF9; "
            "border-radius: 15px; padding: 6px 14px; font-size: 12px; }"
            "QPushButton:hover { background-color: #BBDEFB; }"
        );
        btn->setCursor(Qt::PointingHandCursor);
        connect(btn, &QPushButton::clicked, this, [this, suggestion]() {
            m_inputEdit->setText(suggestion);
            onSendMessage();
            hideSuggestions();
        });
        btnLayout->addWidget(btn);
    }

    btnLayout->addStretch();
    m_suggestionLayout->addLayout(btnLayout);
    m_suggestionArea->show();
}

void MainWindow::hideSuggestions()
{
    m_suggestionArea->hide();
}

void MainWindow::updateTypingIndicator(bool isTyping)
{
    if (isTyping) {
        m_typingLabel->setText("🤖 AI正在思考中...");
        m_typingLabel->show();
    } else {
        m_typingLabel->hide();
    }
}

void MainWindow::saveHistory()
{
    QString filePath = QCoreApplication::applicationDirPath() + "/chat_history.json";
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        QJsonDocument doc(m_messageModel->toJsonArray());
        file.write(doc.toJson());
        file.close();
    }
}

void MainWindow::loadHistory()
{
    QString filePath = QCoreApplication::applicationDirPath() + "/chat_history.json";
    QFile file(filePath);
    if (!file.exists()) return;

    if (file.open(QIODevice::ReadOnly)) {
        QByteArray data = file.readAll();
        file.close();

        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            QJsonArray validMessages;
            for (const QJsonValue &val : doc.array()) {
                if (val.isObject()) {
                    QJsonObject obj = val.toObject();
                    QString content = obj["content"].toString();
                    QString role = obj["role"].toString();
                    // 过滤空内容的消息和无效的系统消息
                    if (!content.trimmed().isEmpty() && 
                        (role == "user" || role == "assistant")) {
                        validMessages.append(val);
                    }
                }
            }

            qDebug() << "加载历史记录:" << doc.array().size() << "条原始消息," 
                     << validMessages.size() << "条有效消息";

            // 如果有无效消息，重写文件
            if (validMessages.size() != doc.array().size()) {
                QFile writeFile(filePath);
                if (writeFile.open(QIODevice::WriteOnly)) {
                    QJsonDocument newDoc(validMessages);
                    writeFile.write(newDoc.toJson());
                    writeFile.close();
                    qDebug() << "已清理无效历史记录";
                }
            }

            if (validMessages.isEmpty()) return;

            m_messageModel->fromJsonArray(validMessages);

            for (const ChatMessage &msg : m_messageModel->messages()) {
                if (msg.isAI || msg.role == "assistant") {
                    createMessageBubble(msg);
                } else {
                    addUserMessage(msg.content, msg.id);
                }
            }
        }
    }
}

void MainWindow::removeMessageBubble(const QString &id)
{
    for (int i = 0; i < m_messageList->count(); ++i) {
        QListWidgetItem *item = m_messageList->item(i);
        if (item->data(Qt::UserRole).toString() == id) {
            QWidget *widget = m_messageList->itemWidget(item);
            if (widget) {
                widget->deleteLater();
            }
            delete item;
            return;
        }
    }
}
