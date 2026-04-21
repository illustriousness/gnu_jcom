#include "appcontroller.h"

#include "payloadcodec.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

namespace {

QString documentsFallbackPath(const QString &fileName)
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath();
    }

    return QDir(baseDir).filePath(fileName);
}

QString ensureParentDirectory(const QString &path)
{
    QFileInfo info(path);
    QDir directory = info.dir();
    if (!directory.exists()) {
        directory.mkpath(QStringLiteral("."));
    }
    return info.absoluteFilePath();
}

} // namespace

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_workspacePath(defaultWorkspacePath())
    , m_logExportPath(defaultLogExportPath())
    , m_logModel(this)
    , m_sendListModel(this)
{
    m_portScanTimer.setInterval(1000);
    m_reconnectTimer.setInterval(2000);
    m_periodicSendTimer.setSingleShot(false);

    connect(&m_portScanTimer, &QTimer::timeout, this, &AppController::scanPorts);
    connect(&m_reconnectTimer, &QTimer::timeout, this, &AppController::triggerReconnect);
    connect(&m_periodicSendTimer, &QTimer::timeout, this, [this]() {
        sendPayload(m_periodicPayload, m_periodicMode);
    });

    connect(&m_serialPort, &QSerialPort::readyRead, this, &AppController::handleReadyRead);
    connect(&m_serialPort, &QSerialPort::errorOccurred, this, &AppController::handlePortError);

    m_portScanTimer.start();
    m_reconnectTimer.start();
    scanPorts();
}

QStringList AppController::availablePorts() const
{
    return m_availablePorts;
}

QString AppController::selectedPort() const
{
    return m_selectedPort;
}

int AppController::baudRate() const
{
    return m_baudRate;
}

int AppController::dataBits() const
{
    return m_dataBits;
}

int AppController::stopBits() const
{
    return m_stopBits;
}

int AppController::parity() const
{
    return m_parity;
}

int AppController::flowControl() const
{
    return m_flowControl;
}

bool AppController::autoReconnect() const
{
    return m_autoReconnect;
}

bool AppController::portOpen() const
{
    return m_serialPort.isOpen();
}

QString AppController::statusMessage() const
{
    return m_statusMessage;
}

QString AppController::lastError() const
{
    return m_lastError;
}

bool AppController::hexDisplay() const
{
    return m_hexDisplay;
}

int AppController::composeMode() const
{
    return m_composeMode;
}

int AppController::lineEnding() const
{
    return m_lineEnding;
}

bool AppController::periodicActive() const
{
    return m_periodicSendTimer.isActive();
}

int AppController::periodicIntervalMs() const
{
    return m_periodicIntervalMs;
}

QString AppController::workspacePath() const
{
    return m_workspacePath;
}

QString AppController::defaultWorkspacePath() const
{
    const QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir directory(baseDir.isEmpty() ? QDir::homePath() : baseDir);
    return directory.filePath(QStringLiteral("workspace.json"));
}

QString AppController::logExportPath() const
{
    return m_logExportPath;
}

QString AppController::defaultLogExportPath() const
{
    return documentsFallbackPath(QStringLiteral("gnu_jcom-session.log"));
}

LogModel *AppController::logModel()
{
    return &m_logModel;
}

SendListModel *AppController::sendListModel()
{
    return &m_sendListModel;
}

void AppController::setSelectedPort(const QString &portName)
{
    if (m_selectedPort == portName) {
        return;
    }

    m_selectedPort = portName;
    emit selectedPortChanged();
}

void AppController::setBaudRate(int baudRate)
{
    if (m_baudRate == baudRate || baudRate <= 0) {
        return;
    }

    m_baudRate = baudRate;
    emit baudRateChanged();
}

void AppController::setDataBits(int dataBits)
{
    if (m_dataBits == dataBits) {
        return;
    }

    m_dataBits = dataBits;
    emit dataBitsChanged();
}

void AppController::setStopBits(int stopBits)
{
    if (m_stopBits == stopBits) {
        return;
    }

    m_stopBits = stopBits;
    emit stopBitsChanged();
}

void AppController::setParity(int parity)
{
    if (m_parity == parity) {
        return;
    }

    m_parity = parity;
    emit parityChanged();
}

void AppController::setFlowControl(int flowControl)
{
    if (m_flowControl == flowControl) {
        return;
    }

    m_flowControl = flowControl;
    emit flowControlChanged();
}

void AppController::setAutoReconnect(bool autoReconnect)
{
    if (m_autoReconnect == autoReconnect) {
        return;
    }

    m_autoReconnect = autoReconnect;
    if (!m_autoReconnect) {
        m_reconnectPending = false;
    }
    emit autoReconnectChanged();
}

void AppController::setHexDisplay(bool hexDisplay)
{
    if (m_hexDisplay == hexDisplay) {
        return;
    }

    m_hexDisplay = hexDisplay;
    emit hexDisplayChanged();
}

void AppController::setComposeMode(int composeMode)
{
    if (m_composeMode == composeMode) {
        return;
    }

    m_composeMode = composeMode;
    emit composeModeChanged();
}

void AppController::setLineEnding(int lineEnding)
{
    if (m_lineEnding == lineEnding) {
        return;
    }

    m_lineEnding = lineEnding;
    emit lineEndingChanged();
}

void AppController::setPeriodicIntervalMs(int intervalMs)
{
    if (intervalMs <= 0 || m_periodicIntervalMs == intervalMs) {
        return;
    }

    m_periodicIntervalMs = intervalMs;
    if (m_periodicSendTimer.isActive()) {
        m_periodicSendTimer.setInterval(m_periodicIntervalMs);
    }
    emit periodicIntervalMsChanged();
}

void AppController::setWorkspacePath(const QString &path)
{
    const QString normalized = normalizedWorkspacePath(path);
    if (normalized.isEmpty() || m_workspacePath == normalized) {
        return;
    }

    m_workspacePath = normalized;
    emit workspacePathChanged();
}

void AppController::setLogExportPath(const QString &path)
{
    const QString normalized = normalizedLogPath(path);
    if (normalized.isEmpty() || m_logExportPath == normalized) {
        return;
    }

    m_logExportPath = normalized;
    emit logExportPathChanged();
}

void AppController::refreshPorts()
{
    scanPorts();
}

bool AppController::openPort()
{
    m_reconnectPending = false;
    return openPortInternal(false);
}

void AppController::closePort()
{
    m_reconnectPending = false;
    m_lastReconnectError.clear();
    stopPeriodicSendInternal(true);

    if (m_serialPort.isOpen()) {
        m_serialPort.close();
        emit portOpenChanged();
        logInfo(QStringLiteral("Disconnected from %1.").arg(m_selectedPort));
    }

    setStatusMessage(QStringLiteral("Disconnected"));
}

bool AppController::sendPayload(const QString &payload, int mode)
{
    if (!m_serialPort.isOpen()) {
        setLastError(QStringLiteral("Open a serial port before sending data."));
        return false;
    }

    QString errorMessage;
    const QByteArray bytes = PayloadCodec::encodePayload(
        payload,
        static_cast<PayloadCodec::PayloadMode>(mode),
        static_cast<PayloadCodec::LineEnding>(m_lineEnding),
        &errorMessage);

    if (!errorMessage.isEmpty()) {
        setLastError(errorMessage);
        return false;
    }

    if (bytes.isEmpty()) {
        setLastError(QStringLiteral("Nothing to send."));
        return false;
    }

    const qint64 queued = m_serialPort.write(bytes);
    if (queued < 0) {
        setLastError(QStringLiteral("Failed to queue payload: %1").arg(m_serialPort.errorString()));
        return false;
    }

    logTransfer(QStringLiteral("tx"), bytes);
    setStatusMessage(QStringLiteral("Sent %1 byte(s) to %2.").arg(bytes.size()).arg(m_selectedPort));
    return true;
}

bool AppController::startPeriodicSend(const QString &payload, int mode, int intervalMs)
{
    if (!m_serialPort.isOpen()) {
        setLastError(QStringLiteral("Open a serial port before starting periodic send."));
        return false;
    }

    if (intervalMs <= 0) {
        setLastError(QStringLiteral("Periodic interval must be greater than zero."));
        return false;
    }

    QString errorMessage;
    PayloadCodec::encodePayload(
        payload,
        static_cast<PayloadCodec::PayloadMode>(mode),
        static_cast<PayloadCodec::LineEnding>(m_lineEnding),
        &errorMessage);

    if (!errorMessage.isEmpty()) {
        setLastError(errorMessage);
        return false;
    }

    m_periodicPayload = payload;
    m_periodicMode = mode;
    setPeriodicIntervalMs(intervalMs);

    if (!m_periodicSendTimer.isActive()) {
        m_periodicSendTimer.start(m_periodicIntervalMs);
        emit periodicActiveChanged();
        logInfo(QStringLiteral("Periodic send started (%1 ms).").arg(m_periodicIntervalMs));
    }

    return true;
}

void AppController::stopPeriodicSend()
{
    stopPeriodicSendInternal(true);
}

bool AppController::addSendItem(const QString &name, const QString &payload, int mode)
{
    if (name.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Send item name cannot be empty."));
        return false;
    }

    if (payload.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Send item payload cannot be empty."));
        return false;
    }

    m_sendListModel.appendItem(name.trimmed(), payload, mode);
    logInfo(QStringLiteral("Saved send item \"%1\".").arg(name.trimmed()));
    return true;
}

bool AppController::updateSendItem(int index, const QString &name, const QString &payload, int mode)
{
    if (name.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Send item name cannot be empty."));
        return false;
    }

    if (payload.trimmed().isEmpty()) {
        setLastError(QStringLiteral("Send item payload cannot be empty."));
        return false;
    }

    if (!m_sendListModel.updateItem(index, name.trimmed(), payload, mode)) {
        setLastError(QStringLiteral("Select a send item before updating."));
        return false;
    }

    logInfo(QStringLiteral("Updated send item \"%1\".").arg(name.trimmed()));
    return true;
}

void AppController::removeSendItem(int index)
{
    const QVariantMap item = m_sendListModel.get(index);
    if (!m_sendListModel.removeItem(index)) {
        setLastError(QStringLiteral("Select a send item before deleting."));
        return;
    }

    logInfo(QStringLiteral("Removed send item \"%1\".").arg(item.value(QStringLiteral("name")).toString()));
}

QVariantMap AppController::sendItemAt(int index) const
{
    return m_sendListModel.get(index);
}

bool AppController::sendSavedItem(int index)
{
    const QVariantMap item = m_sendListModel.get(index);
    if (item.isEmpty()) {
        setLastError(QStringLiteral("Select a send item before sending."));
        return false;
    }

    return sendPayload(item.value(QStringLiteral("payload")).toString(),
                       item.value(QStringLiteral("mode")).toInt());
}

bool AppController::saveWorkspace(const QString &path)
{
    const QString targetPath = normalizedWorkspacePath(path.isEmpty() ? m_workspacePath : path);
    if (targetPath.isEmpty()) {
        setLastError(QStringLiteral("Workspace path is empty."));
        return false;
    }

    const QJsonObject root{
        {QStringLiteral("version"), 1},
        {QStringLiteral("port"),
         QJsonObject{
             {QStringLiteral("selectedPort"), m_selectedPort},
             {QStringLiteral("baudRate"), m_baudRate},
             {QStringLiteral("dataBits"), m_dataBits},
             {QStringLiteral("stopBits"), m_stopBits},
             {QStringLiteral("parity"), m_parity},
             {QStringLiteral("flowControl"), m_flowControl},
             {QStringLiteral("autoReconnect"), m_autoReconnect},
         }},
        {QStringLiteral("ui"),
         QJsonObject{
             {QStringLiteral("hexDisplay"), m_hexDisplay},
             {QStringLiteral("composeMode"), m_composeMode},
             {QStringLiteral("lineEnding"), m_lineEnding},
             {QStringLiteral("periodicIntervalMs"), m_periodicIntervalMs},
             {QStringLiteral("logExportPath"), m_logExportPath},
         }},
        {QStringLiteral("sendList"), m_sendListModel.toJson()},
    };

    QFile file(ensureParentDirectory(targetPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        setLastError(QStringLiteral("Failed to save workspace: %1").arg(file.errorString()));
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    file.close();

    setWorkspacePath(targetPath);
    logInfo(QStringLiteral("Workspace saved to %1.").arg(targetPath));
    return true;
}

bool AppController::loadWorkspace(const QString &path)
{
    const QString targetPath = normalizedWorkspacePath(path.isEmpty() ? m_workspacePath : path);
    QFile file(targetPath);
    if (!file.open(QIODevice::ReadOnly)) {
        setLastError(QStringLiteral("Failed to load workspace: %1").arg(file.errorString()));
        return false;
    }

    const QJsonDocument document = QJsonDocument::fromJson(file.readAll());
    file.close();

    if (!document.isObject()) {
        setLastError(QStringLiteral("Workspace file is not valid JSON."));
        return false;
    }

    const QJsonObject root = document.object();
    const QJsonObject portObject = root.value(QStringLiteral("port")).toObject();
    const QJsonObject uiObject = root.value(QStringLiteral("ui")).toObject();

    setSelectedPort(portObject.value(QStringLiteral("selectedPort")).toString());
    setBaudRate(portObject.value(QStringLiteral("baudRate")).toInt(m_baudRate));
    setDataBits(portObject.value(QStringLiteral("dataBits")).toInt(m_dataBits));
    setStopBits(portObject.value(QStringLiteral("stopBits")).toInt(m_stopBits));
    setParity(portObject.value(QStringLiteral("parity")).toInt(m_parity));
    setFlowControl(portObject.value(QStringLiteral("flowControl")).toInt(m_flowControl));
    setAutoReconnect(portObject.value(QStringLiteral("autoReconnect")).toBool(m_autoReconnect));

    setHexDisplay(uiObject.value(QStringLiteral("hexDisplay")).toBool(m_hexDisplay));
    setComposeMode(uiObject.value(QStringLiteral("composeMode")).toInt(m_composeMode));
    setLineEnding(uiObject.value(QStringLiteral("lineEnding")).toInt(m_lineEnding));
    setPeriodicIntervalMs(uiObject.value(QStringLiteral("periodicIntervalMs")).toInt(m_periodicIntervalMs));
    setLogExportPath(uiObject.value(QStringLiteral("logExportPath")).toString(m_logExportPath));

    m_sendListModel.fromJson(root.value(QStringLiteral("sendList")).toArray());
    setWorkspacePath(targetPath);

    logInfo(QStringLiteral("Workspace loaded from %1.").arg(targetPath));
    return true;
}

bool AppController::saveLog(const QString &path)
{
    const QString targetPath = normalizedLogPath(path.isEmpty() ? m_logExportPath : path);
    if (targetPath.isEmpty()) {
        setLastError(QStringLiteral("Log export path is empty."));
        return false;
    }

    QFile file(ensureParentDirectory(targetPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setLastError(QStringLiteral("Failed to save log: %1").arg(file.errorString()));
        return false;
    }

    file.write(m_logModel.toPlainText().toUtf8());
    file.close();
    setLogExportPath(targetPath);
    logInfo(QStringLiteral("Log exported to %1.").arg(targetPath));
    return true;
}

QString AppController::convertPayloadForMode(const QString &payload, int fromMode, int toMode)
{
    if (fromMode == toMode || payload.isEmpty()) {
        return payload;
    }

    if (fromMode == static_cast<int>(PayloadCodec::PayloadMode::Ascii)
        && toMode == static_cast<int>(PayloadCodec::PayloadMode::Hex)) {
        return PayloadCodec::hexPreview(payload.toUtf8());
    }

    if (fromMode == static_cast<int>(PayloadCodec::PayloadMode::Hex)
        && toMode == static_cast<int>(PayloadCodec::PayloadMode::Ascii)) {
        QString errorMessage;
        const QByteArray bytes = PayloadCodec::encodePayload(
            payload,
            PayloadCodec::PayloadMode::Hex,
            PayloadCodec::LineEnding::None,
            &errorMessage);

        if (!errorMessage.isEmpty()) {
            setLastError(errorMessage);
            return payload;
        }

        return PayloadCodec::asciiPreview(bytes);
    }

    return payload;
}

void AppController::clearLogs()
{
    m_logModel.clear();
    setStatusMessage(QStringLiteral("Log cleared."));
}

void AppController::clearLastError()
{
    if (m_lastError.isEmpty()) {
        return;
    }

    m_lastError.clear();
    emit lastErrorChanged();
}

void AppController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}

void AppController::setLastError(const QString &message)
{
    if (message.isEmpty()) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
    logError(message);
    setStatusMessage(message);
}

void AppController::logInfo(const QString &message)
{
    m_logModel.append(QStringLiteral("info"), message);
}

void AppController::logWarn(const QString &message)
{
    m_logModel.append(QStringLiteral("warn"), message);
}

void AppController::logError(const QString &message)
{
    m_logModel.append(QStringLiteral("error"), message);
}

void AppController::logTransfer(const QString &kind, const QByteArray &bytes)
{
    m_logModel.append(kind, PayloadCodec::asciiPreview(bytes), PayloadCodec::hexPreview(bytes));
}

void AppController::configureSerialPort()
{
    m_serialPort.setPortName(m_selectedPort);
    m_serialPort.setBaudRate(m_baudRate);
    m_serialPort.setDataBits(toDataBitsEnum());
    m_serialPort.setStopBits(toStopBitsEnum());
    m_serialPort.setParity(toParityEnum());
    m_serialPort.setFlowControl(toFlowControlEnum());
}

bool AppController::openPortInternal(bool autoReconnectAttempt)
{
    if (m_selectedPort.isEmpty()) {
        setLastError(QStringLiteral("Select a serial port before opening."));
        return false;
    }

    if (m_serialPort.isOpen()) {
        if (m_serialPort.portName() == m_selectedPort) {
            return true;
        }

        m_serialPort.close();
        emit portOpenChanged();
    }

    configureSerialPort();

    if (!m_serialPort.open(QIODevice::ReadWrite)) {
        const QSerialPortInfo portInfo = portInfoForName(m_selectedPort);
        QString message = QStringLiteral("Failed to open %1: %2").arg(m_selectedPort, m_serialPort.errorString());
        const QString hint = m_serialPort.error() == QSerialPort::PermissionError
            ? permissionHint(portInfo)
            : QString();
        if (!hint.isEmpty()) {
            message += QStringLiteral(" %1").arg(hint);
        }

        if (!autoReconnectAttempt || message != m_lastReconnectError) {
            setLastError(message);
            m_lastReconnectError = message;
        }
        return false;
    }

    m_lastReconnectError.clear();
    m_reconnectPending = false;
    emit portOpenChanged();

    if (autoReconnectAttempt) {
        logInfo(QStringLiteral("Reconnected to %1.").arg(m_selectedPort));
        setStatusMessage(QStringLiteral("Reconnected to %1.").arg(m_selectedPort));
    } else {
        logInfo(QStringLiteral("Connected to %1 @ %2 baud.").arg(m_selectedPort).arg(m_baudRate));
        setStatusMessage(QStringLiteral("Connected to %1.").arg(m_selectedPort));
    }

    clearLastError();
    return true;
}

void AppController::handlePortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError) {
        return;
    }

    switch (error) {
    case QSerialPort::ResourceError:
    case QSerialPort::DeviceNotFoundError:
    case QSerialPort::ReadError:
        logWarn(QStringLiteral("Connection lost on %1: %2").arg(m_selectedPort, m_serialPort.errorString()));
        stopPeriodicSendInternal(true);
        if (m_serialPort.isOpen()) {
            m_serialPort.close();
            emit portOpenChanged();
        }

        if (m_autoReconnect && !m_selectedPort.isEmpty()) {
            m_reconnectPending = true;
            setStatusMessage(QStringLiteral("Port lost, waiting to reconnect %1.").arg(m_selectedPort));
        } else {
            setStatusMessage(QStringLiteral("Port disconnected."));
        }
        break;
    case QSerialPort::PermissionError:
        setLastError(QStringLiteral("Permission error on %1: %2").arg(m_selectedPort, m_serialPort.errorString()));
        break;
    default:
        break;
    }
}

void AppController::handleReadyRead()
{
    const QByteArray bytes = m_serialPort.readAll();
    if (bytes.isEmpty()) {
        return;
    }

    logTransfer(QStringLiteral("rx"), bytes);
    setStatusMessage(QStringLiteral("Received %1 byte(s) from %2.").arg(bytes.size()).arg(m_selectedPort));
}

void AppController::scanPorts()
{
    const QList<QSerialPortInfo> currentInfos = QSerialPortInfo::availablePorts();
    QStringList currentNames;
    currentNames.reserve(currentInfos.size());

    for (const QSerialPortInfo &info : currentInfos) {
        currentNames.append(info.portName());
    }

    for (const QString &portName : currentNames) {
        if (!m_availablePorts.contains(portName)) {
            logInfo(QStringLiteral("Port detected: %1").arg(portName));
            if (m_reconnectPending && portName == m_selectedPort) {
                triggerReconnect();
            }
        }
    }

    for (const QString &portName : m_availablePorts) {
        if (!currentNames.contains(portName)) {
            logWarn(QStringLiteral("Port removed: %1").arg(portName));
            if (portName == m_selectedPort && m_serialPort.isOpen()) {
                stopPeriodicSendInternal(true);
                m_serialPort.close();
                emit portOpenChanged();
                if (m_autoReconnect) {
                    m_reconnectPending = true;
                    setStatusMessage(QStringLiteral("Port %1 removed, waiting to reconnect.").arg(portName));
                } else {
                    setStatusMessage(QStringLiteral("Port %1 removed.").arg(portName));
                }
            }
        }
    }

    if (m_availablePorts != currentNames) {
        m_portInfos = currentInfos;
        m_availablePorts = currentNames;
        emit availablePortsChanged();
    } else {
        m_portInfos = currentInfos;
    }

    if (m_selectedPort.isEmpty() && !m_availablePorts.isEmpty()) {
        setSelectedPort(m_availablePorts.first());
    }
}

void AppController::triggerReconnect()
{
    if (!m_reconnectPending || m_autoReconnectAttemptInFlight || m_selectedPort.isEmpty()) {
        return;
    }

    if (!m_availablePorts.contains(m_selectedPort)) {
        return;
    }

    m_autoReconnectAttemptInFlight = true;
    const bool reopened = openPortInternal(true);
    m_autoReconnectAttemptInFlight = false;

    if (!reopened) {
        setStatusMessage(QStringLiteral("Reconnect pending for %1.").arg(m_selectedPort));
    }
}

void AppController::stopPeriodicSendInternal(bool logEvent)
{
    if (!m_periodicSendTimer.isActive()) {
        return;
    }

    m_periodicSendTimer.stop();
    emit periodicActiveChanged();

    if (logEvent) {
        logInfo(QStringLiteral("Periodic send stopped."));
    }
}

QSerialPortInfo AppController::portInfoForName(const QString &portName) const
{
    for (const QSerialPortInfo &info : m_portInfos) {
        if (info.portName() == portName) {
            return info;
        }
    }

    return {};
}

QString AppController::permissionHint(const QSerialPortInfo &portInfo) const
{
    if (portInfo.portName().isEmpty()) {
        return {};
    }

    const QFileInfo fileInfo(portInfo.systemLocation());
    QString groupHint = QStringLiteral("uucp,lock");
    if (!fileInfo.group().isEmpty() && fileInfo.group() != QStringLiteral("root")) {
        groupHint = fileInfo.group();
        if (groupHint != QStringLiteral("uucp") && groupHint != QStringLiteral("lock")) {
            groupHint += QStringLiteral(",uucp,lock");
        }
    }

    return QStringLiteral("On Arch Linux, serial access commonly requires membership in %1. Example: sudo usermod -aG %1 $USER, then sign out and back in.")
        .arg(groupHint);
}

QSerialPort::DataBits AppController::toDataBitsEnum() const
{
    switch (m_dataBits) {
    case 5:
        return QSerialPort::Data5;
    case 6:
        return QSerialPort::Data6;
    case 7:
        return QSerialPort::Data7;
    case 8:
    default:
        return QSerialPort::Data8;
    }
}

QSerialPort::StopBits AppController::toStopBitsEnum() const
{
    return m_stopBits == 2 ? QSerialPort::TwoStop : QSerialPort::OneStop;
}

QSerialPort::Parity AppController::toParityEnum() const
{
    switch (m_parity) {
    case 1:
        return QSerialPort::EvenParity;
    case 2:
        return QSerialPort::OddParity;
    case 3:
        return QSerialPort::SpaceParity;
    case 4:
        return QSerialPort::MarkParity;
    default:
        return QSerialPort::NoParity;
    }
}

QSerialPort::FlowControl AppController::toFlowControlEnum() const
{
    switch (m_flowControl) {
    case 1:
        return QSerialPort::HardwareControl;
    case 2:
        return QSerialPort::SoftwareControl;
    default:
        return QSerialPort::NoFlowControl;
    }
}

QString AppController::normalizedWorkspacePath(const QString &path) const
{
    if (path.trimmed().isEmpty()) {
        return defaultWorkspacePath();
    }

    return QFileInfo(path).absoluteFilePath();
}

QString AppController::normalizedLogPath(const QString &path) const
{
    if (path.trimmed().isEmpty()) {
        return defaultLogExportPath();
    }

    return QFileInfo(path).absoluteFilePath();
}
