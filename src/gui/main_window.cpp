#include "gui/main_window.h"

#include <QCheckBox>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProcessEnvironment>
#include <QPushButton>
#include <QStandardPaths>
#include <QTextCursor>
#include <QTextStream>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      hostProcess_(new QProcess(this)),
      clientProcess_(new QProcess(this))
{
    BuildUi();
    ApplyStyle();
    WireSignals();
    UpdateStatus("Idle", true);
}

MainWindow::~MainWindow()
{
    if (hostProcess_->state() != QProcess::NotRunning)
    {
        hostProcess_->kill();
        hostProcess_->waitForFinished(1000);
    }
}

void MainWindow::BuildUi()
{
    setWindowTitle("Linkora Simple Client");
    resize(860, 560);

    auto *central = new QWidget(this);
    auto *root = new QVBoxLayout(central);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    auto *header = new QLabel("Linkora One-Click Client");
    header->setObjectName("headerTitle");
    root->addWidget(header);

    auto *subHeader = new QLabel("One app, two buttons: host a network or connect to an existing host");
    subHeader->setObjectName("headerSubtitle");
    root->addWidget(subHeader);

    auto *configBox = new QGroupBox("Quick Setup");
    auto *configGrid = new QGridLayout(configBox);
    configGrid->setHorizontalSpacing(10);
    configGrid->setVerticalSpacing(8);

    auto *hostAddressLabel = new QLabel("Coordinator address");
    hostAddressEdit_ = new QLineEdit("127.0.0.1");

    auto *portLabel = new QLabel("Port");
    portEdit_ = new QLineEdit("38123");

    auto *loginLabel = new QLabel("Network name");
    loginEdit_ = new QLineEdit("room1");

    auto *passwordLabel = new QLabel("Network password");
    passwordEdit_ = new QLineEdit("dev-password");
    passwordEdit_->setEchoMode(QLineEdit::Password);

    tunCheck_ = new QCheckBox("Enable TUN mode (requires privileges)");

    configGrid->addWidget(hostAddressLabel, 0, 0);
    configGrid->addWidget(hostAddressEdit_, 0, 1);
    configGrid->addWidget(portLabel, 0, 2);
    configGrid->addWidget(portEdit_, 0, 3);

    configGrid->addWidget(loginLabel, 1, 0);
    configGrid->addWidget(loginEdit_, 1, 1);
    configGrid->addWidget(passwordLabel, 1, 2);
    configGrid->addWidget(passwordEdit_, 1, 3);

    configGrid->addWidget(tunCheck_, 2, 0, 1, 4);

    root->addWidget(configBox);

    auto *actionsBox = new QGroupBox("Actions");
    auto *actionsRow = new QHBoxLayout(actionsBox);
    actionsRow->setSpacing(10);

    hostButton_ = new QPushButton("Create Network");
    stopHostButton_ = new QPushButton("Stop Host");
    connectButton_ = new QPushButton("Join Network");
    clearLogButton_ = new QPushButton("Clear Log");
    stopHostButton_->setEnabled(false);

    actionsRow->addWidget(hostButton_);
    actionsRow->addWidget(stopHostButton_);
    actionsRow->addWidget(connectButton_);
    actionsRow->addWidget(clearLogButton_);

    root->addWidget(actionsBox);

    statusLabel_ = new QLabel;
    statusLabel_->setObjectName("statusLabel");
    root->addWidget(statusLabel_);

    logEdit_ = new QPlainTextEdit;
    logEdit_->setReadOnly(true);
    root->addWidget(logEdit_, 1);

    setCentralWidget(central);
}

void MainWindow::ApplyStyle()
{
    setStyleSheet(
        "QWidget {"
        "  background: #f7f7f4;"
        "  color: #1d2023;"
        "  font-family: 'IBM Plex Sans', 'Noto Sans', sans-serif;"
        "  font-size: 13px;"
        "}"
        "#headerTitle {"
        "  font-size: 24px;"
        "  font-weight: 700;"
        "  color: #0f1720;"
        "}"
        "#headerSubtitle {"
        "  color: #4a5968;"
        "  margin-bottom: 4px;"
        "}"
        "QGroupBox {"
        "  border: 1px solid #cfd7de;"
        "  border-radius: 10px;"
        "  margin-top: 8px;"
        "  padding-top: 12px;"
        "  font-weight: 600;"
        "  background: #ffffff;"
        "}"
        "QGroupBox::title {"
        "  left: 10px;"
        "  padding: 0 4px;"
        "}"
        "QLineEdit, QPlainTextEdit {"
        "  border: 1px solid #c7d0d8;"
        "  border-radius: 8px;"
        "  padding: 6px 8px;"
        "  background: #ffffff;"
        "}"
        "QPushButton {"
        "  border: none;"
        "  border-radius: 8px;"
        "  padding: 10px 14px;"
        "  background: #1f6feb;"
        "  color: #ffffff;"
        "  font-weight: 600;"
        "}"
        "QPushButton:hover {"
        "  background: #1a60cb;"
        "}"
        "QPushButton:disabled {"
        "  background: #9fb4c9;"
        "}"
        "#statusLabel {"
        "  border-radius: 7px;"
        "  padding: 6px 10px;"
        "  background: #e8eef4;"
        "  font-weight: 600;"
        "}");
}

void MainWindow::WireSignals()
{
    connect(hostButton_, &QPushButton::clicked, this, &MainWindow::StartHost);
    connect(stopHostButton_, &QPushButton::clicked, this, &MainWindow::StopHost);
    connect(connectButton_, &QPushButton::clicked, this, &MainWindow::RunClient);
    connect(clearLogButton_, &QPushButton::clicked, logEdit_, &QPlainTextEdit::clear);

    connect(hostProcess_, &QProcess::readyReadStandardOutput, this, [this]()
            { AppendLog(QString::fromUtf8(hostProcess_->readAllStandardOutput())); });
    connect(hostProcess_, &QProcess::readyReadStandardError, this, [this]()
            { AppendLog(QString::fromUtf8(hostProcess_->readAllStandardError())); });
    connect(hostProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus)
            {
                AppendLog(QString("[host] exited with code %1").arg(code));
                hostButton_->setEnabled(true);
                stopHostButton_->setEnabled(false);
                UpdateStatus("Host stopped", code == 0);
            });

    connect(clientProcess_, &QProcess::readyReadStandardOutput, this, [this]()
            { AppendLog(QString::fromUtf8(clientProcess_->readAllStandardOutput())); });
    connect(clientProcess_, &QProcess::readyReadStandardError, this, [this]()
            { AppendLog(QString::fromUtf8(clientProcess_->readAllStandardError())); });
    connect(clientProcess_, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this,
            [this](int code, QProcess::ExitStatus)
            {
                AppendLog(QString("[client] exited with code %1").arg(code));
                connectButton_->setEnabled(true);
                UpdateStatus(code == 0 ? "Client finished successfully" : "Client failed", code == 0);
            });
}

QString MainWindow::EnsureRuntimeDir() const
{
    QString base = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (base.isEmpty())
    {
        base = QDir::tempPath() + "/linkora";
    }

    QDir dir(base + "/runtime");
    if (!dir.exists())
    {
        (void)dir.mkpath(".");
    }
    return dir.absolutePath();
}

bool MainWindow::WriteHostConfig(QString &outPath, QString &error) const
{
    outPath = EnsureRuntimeDir() + "/host.auto.yaml";
    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        error = "Cannot write host config: " + outPath;
        return false;
    }

    QTextStream s(&file);
    s << "network:\n";
    s << "  name: " << loginEdit_->text().trimmed() << "\n";
    s << "  listen_host: 0.0.0.0\n";
    s << "  listen_port: " << portEdit_->text().trimmed() << "\n\n";
    s << "coordinator:\n";
    s << "  host: " << hostAddressEdit_->text().trimmed() << "\n";
    s << "  port: " << portEdit_->text().trimmed() << "\n\n";
    s << "auth:\n";
    s << "  login: \"" << loginEdit_->text().trimmed() << "\"\n";
    s << "  password: \"" << passwordEdit_->text() << "\"\n\n";
    s << "vpn:\n";
    s << "  virtual_subnet: \"auto\"\n";
    s << "  mtu: 1400\n";
    return true;
}

bool MainWindow::WriteClientConfig(QString &outPath, QString &error) const
{
    outPath = EnsureRuntimeDir() + "/client.auto.yaml";
    QFile file(outPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text))
    {
        error = "Cannot write client config: " + outPath;
        return false;
    }

    QTextStream s(&file);
    s << "network:\n";
    s << "  host: " << hostAddressEdit_->text().trimmed() << "\n";
    s << "  port: " << portEdit_->text().trimmed() << "\n\n";
    s << "auth:\n";
    s << "  login: \"" << loginEdit_->text().trimmed() << "\"\n";
    s << "  password: \"" << passwordEdit_->text() << "\"\n\n";
    s << "vpn:\n";
    s << "  mtu: 1400\n";
    return true;
}

void MainWindow::AppendLog(const QString &line)
{
    logEdit_->moveCursor(QTextCursor::End);
    logEdit_->insertPlainText(line);
    if (!line.endsWith('\n'))
    {
        logEdit_->insertPlainText("\n");
    }
    logEdit_->moveCursor(QTextCursor::End);
}

QString MainWindow::ResolveBinaryPath(const QString &binaryName) const
{
    const QDir dir(QCoreApplication::applicationDirPath());
#if defined(_WIN32)
    const QString name = binaryName + ".exe";
#else
    const QString name = binaryName;
#endif
    const QString direct = dir.filePath(name);
    if (QFileInfo::exists(direct))
    {
        return direct;
    }
    return name;
}

void MainWindow::StartHost()
{
    if (hostProcess_->state() != QProcess::NotRunning)
    {
        UpdateStatus("Host is already running", false);
        return;
    }

    QString cfgPath;
    QString cfgError;
    if (!WriteHostConfig(cfgPath, cfgError))
    {
        UpdateStatus(cfgError, false);
        return;
    }

    const QString hostBin = ResolveBinaryPath("linkora_host");
    QStringList args;
    args << cfgPath;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (tunCheck_->isChecked())
    {
        env.insert("LINKORA_USE_TUN", "1");
    }
    else
    {
        env.remove("LINKORA_USE_TUN");
    }

    hostProcess_->setProcessEnvironment(env);
    hostProcess_->setWorkingDirectory(QDir::currentPath());
    hostProcess_->start(hostBin, args);

    if (!hostProcess_->waitForStarted(2000))
    {
        UpdateStatus("Failed to start host process", false);
        AppendLog("Cannot start host executable: " + hostBin);
        return;
    }

    hostButton_->setEnabled(false);
    stopHostButton_->setEnabled(true);
    UpdateStatus("Host running", true);
    AppendLog("[host] started with generated config: " + cfgPath);
}

void MainWindow::StopHost()
{
    if (hostProcess_->state() == QProcess::NotRunning)
    {
        return;
    }

    hostProcess_->terminate();
    if (!hostProcess_->waitForFinished(1200))
    {
        hostProcess_->kill();
        hostProcess_->waitForFinished(600);
    }
}

void MainWindow::RunClient()
{
    if (clientProcess_->state() != QProcess::NotRunning)
    {
        UpdateStatus("Client is already running", false);
        return;
    }

    QString cfgPath;
    QString cfgError;
    if (!WriteClientConfig(cfgPath, cfgError))
    {
        UpdateStatus(cfgError, false);
        return;
    }

    const QString clientBin = ResolveBinaryPath("linkora_client_worker");
    QStringList args;
    args << cfgPath;

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (tunCheck_->isChecked())
    {
        env.insert("LINKORA_USE_TUN", "1");
    }
    else
    {
        env.remove("LINKORA_USE_TUN");
    }

    clientProcess_->setProcessEnvironment(env);
    clientProcess_->setWorkingDirectory(QDir::currentPath());
    clientProcess_->start(clientBin, args);

    if (!clientProcess_->waitForStarted(2000))
    {
        UpdateStatus("Failed to start client process", false);
        AppendLog("Cannot start client executable: " + clientBin);
        return;
    }

    connectButton_->setEnabled(false);
    UpdateStatus("Client running", true);
    AppendLog("[client] started with generated config: " + cfgPath);
}

void MainWindow::UpdateStatus(const QString &text, bool ok)
{
    statusLabel_->setText(text);
    statusLabel_->setStyleSheet(ok
                                    ? "#statusLabel { background: #deefe4; color: #165b33; }"
                                    : "#statusLabel { background: #f8e2e1; color: #7b1f1f; }");
}
