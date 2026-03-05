#pragma once

#include <QMainWindow>

class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QPushButton;
class QCheckBox;
class QProcess;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    void BuildUi();
    void ApplyStyle();
    void WireSignals();

    void AppendLog(const QString &line);
    QString ResolveBinaryPath(const QString &binaryName) const;
    bool WriteHostConfig(QString &outPath, QString &error) const;
    bool WriteClientConfig(QString &outPath, QString &error) const;
    QString EnsureRuntimeDir() const;

    void StartHost();
    void StopHost();
    void RunClient();
    void UpdateStatus(const QString &text, bool ok);

    QLineEdit *hostAddressEdit_ = nullptr;
    QLineEdit *portEdit_ = nullptr;
    QLineEdit *loginEdit_ = nullptr;
    QLineEdit *passwordEdit_ = nullptr;
    QCheckBox *tunCheck_ = nullptr;
    QLabel *statusLabel_ = nullptr;
    QPlainTextEdit *logEdit_ = nullptr;

    QPushButton *hostButton_ = nullptr;
    QPushButton *stopHostButton_ = nullptr;
    QPushButton *connectButton_ = nullptr;
    QPushButton *clearLogButton_ = nullptr;

    QProcess *hostProcess_ = nullptr;
    QProcess *clientProcess_ = nullptr;
};
