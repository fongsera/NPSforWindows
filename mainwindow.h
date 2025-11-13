#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QProcess>
#include <QString>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 连接服务器的槽函数
    void on_pushButton_clicked();

    // 保存配置到文件的槽函数
    void on_saveButton_clicked();
    
    // 处理进程输出的槽函数
    void readProcessOutput();
    
    // 处理进程错误的槽函数
    void processError(QProcess::ProcessError error);
    
    // 处理进程完成的槽函数
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    
    // UI控件变化的槽函数
    void on_ip_lineEdit_textChanged(const QString &text);
    void on_port_lineEdit_textChanged(const QString &text);
    void on_vkey_lineEdit_textChanged(const QString &text);
    void on_connmode_comboBox_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QProcess *npcProcess; // npc进程对象
    QSettings *settings;  // 配置文件设置对象
    
    // 启动npc进程的方法
    void startNpcProcess(const QString &serverIP, const QString &port, const QString &vkey, const QString &protocol);
    
    // 停止npc进程的方法
    void stopNpcProcess();
    
    // 显示日志信息
    void logMessage(const QString &message);
    
    // 加载样式表的方法
    // void loadStyleSheet(const QString &fileName);
    
    // 加密解密方法
    QByteArray encryptData(const QString &data);
    QString decryptData(const QByteArray &encryptedData);
    
    // 保存和加载设置方法
    void saveSettings();
    void loadSettings();
};
#endif // MAINWINDOW_H