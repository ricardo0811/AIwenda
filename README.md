# AIwenda —— 基于 Qt 6 的 AI 问答桌面客户端

一个使用 Qt 6 / C++17 开发的智能 AI 对话桌面应用,集成大模型对话、流式响应、消息翻译、情感分析、回复建议,以及 TCP 局域网多人聊天功能。

---

## 目录

- [功能特性](#功能特性)
- [项目结构](#项目结构)
- [环境依赖](#环境依赖)
- [快速开始](#快速开始)
- [API 密钥配置](#api-密钥配置)
- [构建说明](#构建说明)
- [使用指南](#使用指南)
- [安全注意事项](#安全注意事项)
- [许可证](#许可证)

---

## 功能特性

- **AI 流式对话**:基于 OpenAI 兼容协议(SiliconFlow),支持 SSE 流式输出,实时显示回复
- **多轮上下文**:自动维护对话历史(默认保留最近 20 条),保持上下文连贯
- **消息翻译**:一键翻译 AI 回复,支持多语言互译
- **情感分析**:对消息进行正面 / 负面 / 中性情感识别并显示置信度
- **智能回复建议**:基于当前对话上下文生成多条候选回复,点击即可填入输入框
- **TCP 局域网聊天**:内置 TCP Server / Client,支持局域网多人实时聊天
- **本地历史持久化**:对话历史可保存到本地 JSON,下次启动恢复
- **气泡式 UI**:仿主流 IM 应用的消息气泡界面,支持打字指示器、建议按钮区

---

## 项目结构

```
AIwenda/
├── AIwenda.pro              # Qt 工程文件
├── main.cpp                 # 程序入口
├── mainwindow.{h,cpp,ui}    # 主窗口与 UI
├── apiconfig.{h,cpp}        # API 配置单例(从 config.ini 加载)
├── chatclient.{h,cpp}       # AI 对话客户端(支持 SSE 流式)
├── messagemodel.{h,cpp}     # 消息数据模型(QAbstractListModel)
├── translationservice.{h,cpp}   # 翻译服务
├── sentimentanalyzer.{h,cpp}    # 情感分析服务
├── replysuggester.{h,cpp}       # 智能回复建议
├── tcpchatserver.{h,cpp}        # TCP 局域网聊天服务端
├── tcpchatclient.{h,cpp}        # TCP 局域网聊天客户端
├── config.ini.example       # 配置文件模板(不含真实密钥)
└── .gitignore
```

---

## 环境依赖

| 依赖项     | 版本要求          |
| ---------- | ----------------- |
| Qt         | 6.5 或更高(本仓库基于 Qt 6.11 开发) |
| C++ 编译器 | 支持 C++17(MSVC 2022 64-bit 推荐,MinGW 亦可) |
| 构建工具   | qmake 或 CMake    |
| 网络       | 需要可访问 SiliconFlow API(`https://api.siliconflow.cn`) |

Qt 模块依赖:`Qt::Widgets`、`Qt::Network`。

---

## 快速开始

```bash
# 1. 克隆仓库
git clone https://github.com/ricardo0811/AIwenda.git
cd AIwenda

# 2. 准备配置文件(见下一节)
copy config.ini.example config.ini
# 然后用文本编辑器打开 config.ini,填入真实 API Key

# 3. 使用 Qt Creator 打开 AIwenda.pro
#    选择 Kit (推荐 MSVC 2022 64-bit) -> 构建 -> 运行
```

---

## API 密钥配置

本应用通过 `config.ini` 文件加载 API 配置,**该文件已被 `.gitignore` 忽略,不会上传到仓库**。

### 1. 申请 API Key

前往 [SiliconFlow 云平台](https://cloud.siliconflow.cn)注册账号,在控制台创建 API Key(格式以 `sk-` 开头)。

### 2. 创建 config.ini

将仓库根目录下的 `config.ini.example` 复制为 `config.ini`,并替换其中的 `key` 字段:

```ini
[api]
key=sk-你的真实API密钥
endpoint=https://api.siliconflow.cn/v1/chat/completions
model=deepseek-ai/DeepSeek-V3
translate_endpoint=https://api.siliconflow.cn/v1/chat/completions
sentiment_endpoint=https://api.siliconflow.cn/v1/chat/completions
```

### 3. config.ini 查找路径

程序启动时会按以下顺序查找 `config.ini`,找到第一个即加载:

1. 可执行文件所在目录(`applicationDirPath()/config.ini`)
2. 上一级目录
3. 上两级目录(项目根目录)
4. 上三级目录

> 推荐:把 `config.ini` 放在项目根目录,无论在 Debug / Release / 任意构建目录下运行都能被找到。

### 4. 支持的模型示例

SiliconFlow 平台支持多款模型,可在 `config.ini` 的 `model` 字段中切换:

- `deepseek-ai/DeepSeek-V3`
- `deepseek-ai/DeepSeek-R1`
- `Qwen/Qwen2.5-7B-Instruct`
- `meta-llama/Meta-Llama-3.1-8B-Instruct`

完整模型列表见 [SiliconFlow 文档](https://docs.siliconflow.cn/cn/userguide/capabilities/text-generation)。

---

## 构建说明

### 方式一:Qt Creator(推荐)

1. 打开 `AIwenda.pro`
2. 选择 Kit(推荐 **MSVC 2022 64-bit**)
3. 点击左下角 ▶ 运行,或 `Ctrl+R`

### 方式二:命令行

```bash
# 使用 qmake
qmake AIwenda.pro
nmake          # MSVC
# 或
mingw32-make   # MinGW
```

生成的可执行文件位于 `build/Desktop_Qt_*` 目录下,首次运行前请确保 `config.ini` 已放置在上述查找路径之一。

---

## 使用指南

启动程序后,主界面分为左右两部分:

- **左侧聊天面板**:与 AI 实时对话,支持流式输出、清空对话、重新生成上一条 AI 回复
- **右侧侧边面板**:包含翻译显示区与情感分析结果

主要交互:

- 在底部输入框输入消息,按回车或点击「发送」
- 勾选「翻译」复选框后,AI 回复将自动翻译并显示在右侧
- AI 回复完成后,下方可能出现「回复建议」按钮,点击可快速填入输入框
- 点击「重新生成」可让 AI 重新生成上一条回复

---

## 安全注意事项

- **请勿将真实 API 密钥提交到 Git 仓库**。`config.ini` 已在 `.gitignore` 中排除
- **请勿在源码中硬编码密钥**。本仓库已移除所有硬编码密钥,占位符为 `YOUR_API_KEY_HERE`
- 若不慎提交了真实密钥,请立即在 SiliconFlow 控制台**吊销并重新生成**该密钥
- 建议为不同环境(开发 / 测试 / 生产)使用独立的 API Key,并设置调用额度上限

---

## 许可证

本项目仅供学习与交流使用。如需商业使用,请联系作者。
