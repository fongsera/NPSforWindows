#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDateTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QCoreApplication>
#include <QFile>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , npcProcess(nullptr)
    , settings(nullptr)
{
    ui->setupUi(this);

    // 设置窗口大小
    resize(400, 200);
    setWindowTitle("NPS for Windows");
    setWindowIcon(QIcon(":/styles/logo.svg"));
    // 初始化QSettings对象
    QString configFilePath = QCoreApplication::applicationDirPath() + "/config.ini";
    settings = new QSettings(configFilePath, QSettings::IniFormat);

    // 从配置文件加载设置
    loadSettings();

    // 允许按钮切换状态
    ui->pushButton->setCheckable(true);

    // 清空日志区域
    ui->textEdit->clear();
    logMessage("程序启动，等待连接服务器...");
}

MainWindow::~MainWindow()
{
    // 确保在程序退出时停止npc进程
    stopNpcProcess();

    // 释放资源
    delete settings;
    delete ui;
}


// 简单的加密方法实现
QByteArray MainWindow::encryptData(const QString &data)
{
    // 使用一个简单的异或加密 + Base64编码
    QByteArray bytes = data.toUtf8();

    // 使用固定密钥进行异或操作
    const QByteArray key = "my_secret_key_12345";
    for (int i = 0; i < bytes.size(); ++i) {
        bytes[i] = bytes[i] ^ key[i % key.size()];
    }

    // 返回Base64编码的加密数据
    return bytes.toBase64();
}

// 简单的解密方法实现
QString MainWindow::decryptData(const QByteArray &encryptedData)
{
    // 解码Base64数据
    QByteArray bytes = QByteArray::fromBase64(encryptedData);

    // 使用固定密钥进行异或操作
    const QByteArray key = "my_secret_key_12345";
    for (int i = 0; i < bytes.size(); ++i) {
        bytes[i] = bytes[i] ^ key[i % key.size()];
    }

    // 返回解密后的字符串
    return QString::fromUtf8(bytes);
}

// 保存设置到配置文件
void MainWindow::saveSettings()
{
    if (!settings) return; // 安全检查

    // 保存普通设置项
    settings->setValue("Connection/serverIP", ui->ip_lineEdit->text());
    settings->setValue("Connection/port", ui->port_lineEdit->text());

    // 加密保存vkey
    QString vkey = ui->vkey_lineEdit->text();
    if (!vkey.isEmpty()) {
        QByteArray encryptedVkey = encryptData(vkey);
        settings->setValue("Connection/encryptedVkey", encryptedVkey);
    } else {
        settings->setValue("Connection/encryptedVkey", "");
    }

    // 保存协议索引
    settings->setValue("Connection/protocolIndex", ui->connmode_comboBox->currentIndex());

    // 确保立即写入文件
    settings->sync();

    logMessage("配置已保存到文件: " + settings->fileName());
}

// 从配置文件加载设置
void MainWindow::loadSettings()
{
    if (!settings) return; // 安全检查

    // 添加调试信息
    logMessage("开始加载配置文件...");

    // 检查配置文件是否存在
    QString configFilePath = QCoreApplication::applicationDirPath() + "/config.ini";
    QFileInfo configFile(configFilePath);

    if (configFile.exists()) {
        logMessage("找到配置文件，正在加载...");

        // 设置默认值并加载普通配置
        ui->ip_lineEdit->setText(settings->value("Connection/serverIP", "127.0.0.1").toString());
        ui->port_lineEdit->setText(settings->value("Connection/port", "8080").toString());

        // 解密加载vkey
        QByteArray encryptedVkey = settings->value("Connection/encryptedVkey", "").toByteArray();
        if (!encryptedVkey.isEmpty()) {
            QString decryptedVkey = decryptData(encryptedVkey);
            ui->vkey_lineEdit->setText(decryptedVkey);
            logMessage("成功解密并加载vkey");
        } else {
            ui->vkey_lineEdit->setText("test"); // 默认值
            logMessage("未找到加密的vkey，使用默认值");
        }

        // 加载协议索引
        ui->connmode_comboBox->setCurrentIndex(settings->value("Connection/protocolIndex", 0).toInt());

        logMessage("配置已从保存的设置中加载完成");
    } else {
        logMessage("未找到配置文件，使用默认配置");
        // 使用默认值
        ui->ip_lineEdit->setText("127.0.0.1");
        ui->port_lineEdit->setText("8080");
        ui->vkey_lineEdit->setText("test");
        ui->connmode_comboBox->setCurrentIndex(0);
    }

    // 设置占位符
    ui->ip_lineEdit->setPlaceholderText("请输入服务器IP地址");
    ui->port_lineEdit->setPlaceholderText("请输入端口号");
    ui->vkey_lineEdit->setPlaceholderText("请输入验证密钥");
}

// 保存配置按钮的槽函数
void MainWindow::on_saveButton_clicked()
{
    saveSettings();
    QMessageBox::information(this, "保存成功", "配置已成功保存到文件");
    logMessage("配置已手动保存到文件");
}

// 这些槽函数现在只是空实现
void MainWindow::on_ip_lineEdit_textChanged(const QString &text)
{
    Q_UNUSED(text);
}

void MainWindow::on_port_lineEdit_textChanged(const QString &text)
{
    Q_UNUSED(text);
}

void MainWindow::on_vkey_lineEdit_textChanged(const QString &text)
{
    Q_UNUSED(text);
}

void MainWindow::on_connmode_comboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index);
}

void MainWindow::startNpcProcess(const QString &serverIP, const QString &port, const QString &vkey, const QString &protocol)
{
    // 停止现有进程
    stopNpcProcess();

    // 创建新的进程对象
    npcProcess = new QProcess(this);

    // 连接信号槽
    connect(npcProcess, &QProcess::readyReadStandardOutput, this, &MainWindow::readProcessOutput);
    connect(npcProcess, &QProcess::readyReadStandardError, this, &MainWindow::readProcessOutput);
    connect(npcProcess, &QProcess::errorOccurred, this, &MainWindow::processError);
    connect(npcProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &MainWindow::processFinished);

    // 构建npc的启动参数
    QStringList arguments;
    // npc的启动参数格式示例: -server=127.0.0.1:8080 -vkey=test -type=tcp
    arguments << "-server=" + serverIP + ":" + port;
    arguments << "-vkey=" + vkey;
    arguments << "-type=" + protocol;

    // 检查npc.exe是否存在
    QFileInfo npcFile("npc.exe");
    if (!npcFile.exists()) {
        logMessage("错误: npc.exe 文件不存在，请确保该文件在程序目录下");
        QMessageBox::critical(this, "文件不存在", "npc.exe 文件不存在，请确保该文件在程序目录下");
        ui->pushButton->setChecked(false);
        delete npcProcess;
        npcProcess = nullptr;
        // 更新按钮状态
        ui->pushButton->setText("连接服务器"); 
        return;
    }

    // 启动npc进程
    logMessage("正在启动npc进程...");
    logMessage(QString("执行命令: npc.exe %1").arg(arguments.join(" ")));
    npcProcess->start("npc.exe", arguments);

    // 检查启动是否成功
    if (!npcProcess->waitForStarted(3000)) {
        logMessage("错误: 无法启动npc进程");
        QMessageBox::critical(this, "启动失败", "无法启动npc进程，请检查npc.exe是否可用");
        ui->pushButton->setChecked(false);
        delete npcProcess;
        npcProcess = nullptr;
        // 更新按钮状态
        ui->pushButton->setText("连接服务器");  
        return;  
    } 
    else 
    {
        logMessage("npc进程已成功启动");
        statusBar()->showMessage("内网穿透服务已连接", 3000);
    }
}

void MainWindow::stopNpcProcess()
{
    if (npcProcess != nullptr) {
        if (npcProcess->state() == QProcess::Running) {
            logMessage("正在终止npc进程...");

            npcProcess->terminate();

            // 如果3秒内没有终止，则强制杀死
            if (!npcProcess->waitForFinished(3000)) {
                logMessage("警告: 强制终止npc进程");
                npcProcess->kill();
                npcProcess->waitForFinished(1000);
            }
        }

        // 断开所有连接并删除进程对象
        disconnect(npcProcess, nullptr, nullptr, nullptr);
        delete npcProcess;
        npcProcess = nullptr;

        logMessage("npc进程已停止");
        statusBar()->showMessage("内网穿透服务已断开", 3000);
    }
}

void MainWindow::readProcessOutput()
{
    if (npcProcess != nullptr) {
        // 读取标准输出
        QByteArray output = npcProcess->readAllStandardOutput();
        if (!output.isEmpty()) {
            logMessage(QString::fromLocal8Bit(output).trimmed());
        }

        // 读取标准错误
        QByteArray errorOutput = npcProcess->readAllStandardError();
        if (!errorOutput.isEmpty()) {
            logMessage("错误: " + QString::fromLocal8Bit(errorOutput).trimmed());
        }
    }
}

void MainWindow::processError(QProcess::ProcessError error)
{
    QString errorMsg;

    switch (error) {
    case QProcess::FailedToStart:
        errorMsg = "进程启动失败: npc.exe可能不存在或权限不足";
        break;
    case QProcess::Crashed:
        errorMsg = "进程崩溃";
        break;
    case QProcess::Timedout:
        errorMsg = "进程超时";
        break;
    case QProcess::WriteError:
        errorMsg = "写入错误: 无法向进程写入数据";
        break;
    case QProcess::ReadError:
        errorMsg = "读取错误: 无法从进程读取数据";
        break;
    case QProcess::UnknownError:
    default:
        errorMsg = "未知错误";
        break;
    }

    logMessage("进程错误: " + errorMsg);
    statusBar()->showMessage("内网穿透服务错误", 3000);

    // 重置按钮状态
    if (ui->pushButton->isChecked()) {
        ui->pushButton->setChecked(false);
        ui->pushButton->setText("连接服务器");
    }
}

void MainWindow::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    QString statusMsg = (exitStatus == QProcess::NormalExit) ? "正常退出" : "异常退出";
    logMessage(QString("npc进程已结束 (%1, 退出码: %2)").arg(statusMsg).arg(exitCode));

    // 重置按钮状态
    if (ui->pushButton->isChecked()) {
        ui->pushButton->setChecked(false);
        ui->pushButton->setText("连接服务器");
        statusBar()->showMessage("内网穿透服务已断开", 3000);
    }
}

void MainWindow::logMessage(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
    ui->textEdit->append(QString("[%1] %2").arg(timestamp).arg(message));

    // 自动滚动到最新消息
    QTextCursor cursor = ui->textEdit->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEdit->setTextCursor(cursor);
}

void MainWindow::on_pushButton_clicked()
{
    if (ui->pushButton->isChecked()) {
        // 检查是否已经有进程在运行
        if (npcProcess != nullptr && npcProcess->state() == QProcess::Running) {
            logMessage("警告: npc进程已经在运行中");
            return;
        }

        // 获取用户输入的参数
        QString serverIP = ui->ip_lineEdit->text().trimmed();
        QString port = ui->port_lineEdit->text().trimmed();
        QString vkey = ui->vkey_lineEdit->text().trimmed();
        QString protocol = ui->connmode_comboBox->currentText().toLower();

        // 参数验证
        if (serverIP.isEmpty() || port.isEmpty() || vkey.isEmpty()) {
            QMessageBox::warning(this, "参数错误", "请填写完整的服务器信息");
            ui->pushButton->setChecked(false);
            return;
        }
        
        // 更新按钮状态
        ui->pushButton->setText("断开连接");

        // 启动npc进程
        logMessage("正在启动内网穿透服务...");
        logMessage(QString("连接配置: %1:%2, 协议: %3, 密钥: %4").arg(serverIP).arg(port).arg(protocol.toUpper()).arg(vkey));
        startNpcProcess(serverIP, port, vkey, protocol);


    } else {
        // 停止npc进程
        logMessage("正在停止内网穿透服务...");
        stopNpcProcess();

        // 更新按钮状态
        ui->pushButton->setText("连接服务器");
    }
}
