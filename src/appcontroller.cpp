#include "appcontroller.h"

#include "payloadcodec.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

#include <limits>

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

QStringList repeatedStringList(int count, const QString &value)
{
    QStringList values;
    values.reserve(count);
    for (int index = 0; index < count; ++index) {
        values.append(value);
    }
    return values;
}

QVector<PacketFieldDef> defaultPacketFields()
{
    auto makeField = [](const QString &name, const QString &defaultValue) {
        PacketFieldDef field;
        field.name = name;
        field.type = PacketValueType::UInt;
        field.typeName = QStringLiteral("uint");
        field.byteWidth = 2;
        field.endian = QStringLiteral("little");
        field.scale = 1.0;
        field.precision = 0;
        field.defaultValue = defaultValue;
        return field;
    };

    return {
        makeField(QStringLiteral("x"), QStringLiteral("1")),
        makeField(QStringLiteral("x2"), QStringLiteral("2")),
        makeField(QStringLiteral("x3"), QStringLiteral("3")),
    };
}

} // namespace

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_workspacePath(defaultWorkspacePath())
    , m_logExportPath(defaultLogExportPath())
    , m_packetSchemaPath(defaultPacketSchemaPath())
    , m_logModel(this)
    , m_sendListModel(this)
    , m_packetFieldModel(this)
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

    m_packetFieldModel.setFieldDefinitions(defaultPacketFields());
    PacketSchema initialSchema;
    if (buildEditorPacketSchema(&initialSchema, nullptr)) {
        setPacketRuntimeSchema(initialSchema, false);
    }
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

QString AppController::packetSchemaPath() const
{
    return m_packetSchemaPath;
}

QString AppController::packetSchemaName() const
{
    return m_packetSchemaName;
}

QString AppController::packetHeaderText() const
{
    return m_packetHeaderText;
}

QString AppController::packetFooterText() const
{
    return m_packetFooterText;
}

QString AppController::packetChecksum() const
{
    return m_packetChecksum;
}

bool AppController::packetSchemaLoaded() const
{
    return m_packetSchema.isValid();
}

int AppController::parsedFrameCount() const
{
    return m_parsedFrameCount;
}

QStringList AppController::chartSeriesNames() const
{
    return m_chartSeriesNames;
}

double AppController::chartXMin() const
{
    return m_chartXMin;
}

double AppController::chartXMax() const
{
    return m_chartXMax;
}

double AppController::chartYMin() const
{
    return m_chartYMin;
}

double AppController::chartYMax() const
{
    return m_chartYMax;
}

LogModel *AppController::logModel()
{
    return &m_logModel;
}

SendListModel *AppController::sendListModel()
{
    return &m_sendListModel;
}

PacketFieldModel *AppController::packetFieldModel()
{
    return &m_packetFieldModel;
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

void AppController::setPacketSchemaPath(const QString &path)
{
    const QString normalized = normalizedPacketSchemaPath(path);
    if (normalized.isEmpty() || m_packetSchemaPath == normalized) {
        return;
    }

    m_packetSchemaPath = normalized;
    emit packetSchemaPathChanged();
}

void AppController::setPacketSchemaName(const QString &name)
{
    const QString normalized = name.trimmed();
    if (normalized.isEmpty() || m_packetSchemaName == normalized) {
        return;
    }

    m_packetSchemaName = normalized;
    emit packetSchemaNameChanged();
}

void AppController::setPacketHeaderText(const QString &text)
{
    if (m_packetHeaderText == text) {
        return;
    }

    m_packetHeaderText = text;
    emit packetHeaderTextChanged();
}

void AppController::setPacketFooterText(const QString &text)
{
    if (m_packetFooterText == text) {
        return;
    }

    m_packetFooterText = text;
    emit packetFooterTextChanged();
}

void AppController::setPacketChecksum(const QString &checksum)
{
    const QString normalized = checksum.trimmed().toLower();
    if (normalized.isEmpty() || !isSupportedPacketChecksum(normalized)) {
        return;
    }

    if (m_packetChecksum == normalized) {
        return;
    }

    m_packetChecksum = normalized;
    emit packetChecksumChanged();
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

    return sendBytesDirect(bytes, QStringLiteral("tx"), QString());
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

    QJsonArray packetSendValues;
    const QStringList fieldValues = m_packetFieldModel.sendValues();
    for (const QString &value : fieldValues) {
        packetSendValues.append(value);
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
        {QStringLiteral("packet"),
         QJsonObject{
             {QStringLiteral("schemaPath"), m_packetSchemaPath},
             {QStringLiteral("definition"), packetEditorToJsonObject()},
             {QStringLiteral("sendValues"), packetSendValues},
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
    const QJsonObject packetObject = root.value(QStringLiteral("packet")).toObject();

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

    const QString schemaPath = packetObject.value(QStringLiteral("schemaPath")).toString();
    if (!schemaPath.trimmed().isEmpty()) {
        setPacketSchemaPath(schemaPath);
    }
    const QJsonObject packetDefinition = packetObject.value(QStringLiteral("definition")).toObject();
    if (!packetDefinition.isEmpty()) {
        loadPacketEditorFromJsonObject(packetDefinition);
        applyPacketDefinition();
    } else if (!schemaPath.trimmed().isEmpty() && !loadPacketSchema(schemaPath)) {
        return false;
    }

    QStringList packetSendValues;
    const QJsonArray packetSendArray = packetObject.value(QStringLiteral("sendValues")).toArray();
    packetSendValues.reserve(packetSendArray.size());
    for (const QJsonValue &value : packetSendArray) {
        packetSendValues.append(value.toString());
    }
    if (!packetSendValues.isEmpty()) {
        m_packetFieldModel.setSendValues(packetSendValues);
    }

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

bool AppController::loadPacketSchema(const QString &path)
{
    const QString targetPath = normalizedPacketSchemaPath(path.isEmpty() ? m_packetSchemaPath : path);
    if (targetPath.isEmpty()) {
        setLastError(QStringLiteral("Packet schema path is empty."));
        return false;
    }

    PacketSchema schema;
    QString errorMessage;
    if (!loadPacketSchemaFromFile(targetPath, &schema, &errorMessage)) {
        setLastError(QStringLiteral("Failed to load packet schema: %1").arg(errorMessage));
        return false;
    }

    setPacketSchemaPath(targetPath);
    loadPacketEditorFromJsonObject(packetSchemaToJsonObject(schema));
    setPacketRuntimeSchema(schema, true);
    clearLastError();
    return true;
}

bool AppController::savePacketSchema(const QString &path)
{
    const QString targetPath = normalizedPacketSchemaPath(path.isEmpty() ? m_packetSchemaPath : path);
    if (targetPath.isEmpty()) {
        setLastError(QStringLiteral("Packet schema path is empty."));
        return false;
    }

    PacketSchema schema;
    QString errorMessage;
    if (!buildEditorPacketSchema(&schema, &errorMessage)) {
        setLastError(errorMessage);
        return false;
    }

    QFile file(ensureParentDirectory(targetPath));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate | QIODevice::Text)) {
        setLastError(QStringLiteral("Failed to save packet schema: %1").arg(file.errorString()));
        return false;
    }

    file.write(QJsonDocument(packetSchemaToJsonObject(schema)).toJson(QJsonDocument::Indented));
    file.close();

    setPacketSchemaPath(targetPath);
    clearLastError();
    logInfo(QStringLiteral("Packet schema exported to %1.").arg(targetPath));
    setStatusMessage(QStringLiteral("Packet schema saved to %1.").arg(targetPath));
    return true;
}

bool AppController::applyPacketDefinition()
{
    PacketSchema schema;
    QString errorMessage;
    if (!buildEditorPacketSchema(&schema, &errorMessage)) {
        setLastError(errorMessage);
        return false;
    }

    setPacketRuntimeSchema(schema, true);
    clearLastError();
    return true;
}

QString AppController::buildPacketPreview()
{
    PacketSchema schema;
    QString errorMessage;
    if (!buildEditorPacketSchema(&schema, &errorMessage)) {
        return {};
    }

    return buildPacketHexPreview(schema, m_packetFieldModel.sendValues(), &errorMessage);
}

bool AppController::sendPacket()
{
    PacketSchema schema;
    QString errorMessage;
    if (!buildEditorPacketSchema(&schema, &errorMessage)) {
        setLastError(errorMessage);
        return false;
    }

    const QByteArray frame = buildPacketFrame(schema, m_packetFieldModel.sendValues(), &errorMessage);
    if (frame.isEmpty()) {
        setLastError(errorMessage.isEmpty()
                         ? QStringLiteral("Failed to build packet frame.")
                         : errorMessage);
        return false;
    }

    return sendBytesDirect(frame, QStringLiteral("tx"), QStringLiteral("Packet frame"));
}

bool AppController::buildEditorPacketSchema(PacketSchema *schema, QString *errorMessage) const
{
    return buildPacketSchema(m_packetSchemaName,
                             m_packetHeaderText,
                             m_packetFooterText,
                             m_packetChecksum,
                             m_packetFieldModel.fieldDefinitions(),
                             schema,
                             errorMessage);
}

void AppController::setPacketRuntimeSchema(const PacketSchema &schema, bool logEvent)
{
    const QString previousRuntimeName = m_packetSchema.name;
    const bool previousLoaded = m_packetSchema.isValid();
    const QStringList nextChartSeriesNames = schema.chartFieldNames();
    const bool chartSeriesChanged = m_chartSeriesNames != nextChartSeriesNames;

    m_packetSchema = schema;
    m_chartSeriesNames = nextChartSeriesNames;
    resetPacketRuntimeState();

    if (previousLoaded != m_packetSchema.isValid()) {
        emit packetSchemaLoadedChanged();
    }
    if (chartSeriesChanged) {
        emit chartSeriesNamesChanged();
    }

    if (logEvent) {
        logInfo(QStringLiteral("Packet parser applied: %1 (%2 field(s)).")
                    .arg(m_packetSchema.name)
                    .arg(m_packetSchema.fields.size()));
        setStatusMessage(QStringLiteral("Packet parser %1 applied.").arg(m_packetSchema.name));
    } else if (previousRuntimeName != m_packetSchema.name) {
        setStatusMessage(QStringLiteral("Packet parser %1 ready.").arg(m_packetSchema.name));
    }
}

void AppController::loadPacketEditorFromJsonObject(const QJsonObject &object)
{
    setPacketSchemaName(object.value(QStringLiteral("name")).toString(m_packetSchemaName));
    setPacketHeaderText(object.value(QStringLiteral("header")).toString(m_packetHeaderText));
    setPacketFooterText(object.value(QStringLiteral("footer")).toString(m_packetFooterText));
    setPacketChecksum(object.value(QStringLiteral("checksum")).toString(m_packetChecksum));

    QVector<PacketFieldDef> fields;
    const QJsonArray fieldArray = object.value(QStringLiteral("fields")).toArray();
    fields.reserve(fieldArray.size());

    for (const QJsonValue &value : fieldArray) {
        const QJsonObject fieldObject = value.toObject();
        PacketFieldDef field;
        field.name = fieldObject.value(QStringLiteral("name")).toString();
        field.typeName = fieldObject.value(QStringLiteral("type")).toString(QStringLiteral("uint")).trimmed().toLower();
        field.byteWidth = fieldObject.value(QStringLiteral("byteWidth"))
                              .toInt(fieldObject.value(QStringLiteral("bytes")).toInt(0));
        field.endian = fieldObject.value(QStringLiteral("endian")).toString(QStringLiteral("little")).trimmed().toLower();
        field.scale = fieldObject.value(QStringLiteral("scale")).toDouble(1.0);
        field.precision = fieldObject.contains(QStringLiteral("precision"))
            ? fieldObject.value(QStringLiteral("precision")).toInt()
            : -1;
        field.unit = fieldObject.value(QStringLiteral("unit")).toString();
        field.chart = fieldObject.value(QStringLiteral("chart")).toBool(false);
        field.defaultValue = fieldObject.value(QStringLiteral("defaultValue")).toString(QStringLiteral("0"));

        if (!normalizePacketFieldDef(&field, nullptr)) {
            field.name = field.name.trimmed().isEmpty()
                ? QStringLiteral("field%1").arg(fields.size() + 1)
                : field.name.trimmed();
            field.type = PacketValueType::UInt;
            field.typeName = QStringLiteral("uint");
            field.byteWidth = 2;
            field.endian = QStringLiteral("little");
            field.scale = 1.0;
            field.precision = 0;
            field.defaultValue = QStringLiteral("0");
        }
        if (qFuzzyIsNull(field.scale)) {
            field.scale = 1.0;
        }
        if (field.name.trimmed().isEmpty()) {
            field.name = QStringLiteral("field%1").arg(fields.size() + 1);
        }
        fields.append(field);
    }

    if (fields.isEmpty()) {
        fields = defaultPacketFields();
    }

    m_packetFieldModel.setFieldDefinitions(fields);
}

QJsonObject AppController::packetEditorToJsonObject() const
{
    QJsonArray fieldArray;
    const QVector<PacketFieldDef> fields = m_packetFieldModel.fieldDefinitions();

    for (const PacketFieldDef &field : fields) {
        fieldArray.append(QJsonObject{
            {QStringLiteral("name"), field.name},
            {QStringLiteral("type"), field.typeName},
            {QStringLiteral("byteWidth"), field.byteWidth},
            {QStringLiteral("endian"), field.endian},
            {QStringLiteral("scale"), field.scale},
            {QStringLiteral("precision"), field.precision},
            {QStringLiteral("unit"), field.unit},
            {QStringLiteral("chart"), field.chart},
            {QStringLiteral("defaultValue"), field.defaultValue},
        });
    }

    return QJsonObject{
        {QStringLiteral("name"), m_packetSchemaName},
        {QStringLiteral("header"), m_packetHeaderText},
        {QStringLiteral("footer"), m_packetFooterText},
        {QStringLiteral("checksum"), m_packetChecksum},
        {QStringLiteral("fields"), fieldArray},
    };
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

bool AppController::sendBytesDirect(const QByteArray &bytes,
                                    const QString &kindLabel,
                                    const QString &statusLabel)
{
    if (!m_serialPort.isOpen()) {
        setLastError(QStringLiteral("Open a serial port before sending data."));
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

    logTransfer(kindLabel, bytes);
    clearLastError();

    if (statusLabel.isEmpty()) {
        setStatusMessage(QStringLiteral("Sent %1 byte(s) to %2.").arg(bytes.size()).arg(m_selectedPort));
    } else {
        setStatusMessage(QStringLiteral("%1 sent %2 byte(s) to %3.")
                             .arg(statusLabel)
                             .arg(bytes.size())
                             .arg(m_selectedPort));
    }
    return true;
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
    resetPacketRuntimeState();
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
    processPacketBytes(bytes);
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

void AppController::resetPacketRuntimeState()
{
    m_packetRxBuffer.clear();

    if (m_parsedFrameCount != 0) {
        m_parsedFrameCount = 0;
        emit parsedFrameCountChanged();
    }

    m_packetFieldModel.updateReceivedValues(repeatedStringList(m_packetFieldModel.count(), QStringLiteral("-")));

    m_chartPoints.clear();
    m_chartPoints.resize(m_chartSeriesNames.size());
    m_chartXMin = 0.0;
    m_chartXMax = static_cast<double>(m_chartPointWindow);
    m_chartYMin = -1.0;
    m_chartYMax = 1.0;
    emit chartReset();
    emit chartRangeChanged();
}

void AppController::processPacketBytes(const QByteArray &bytes)
{
    if (!m_packetSchema.isValid() || bytes.isEmpty()) {
        return;
    }

    const QByteArray header = m_packetSchema.header;
    const int frameSize = m_packetSchema.frameSize();
    if (header.isEmpty() || frameSize <= 0) {
        return;
    }

    m_packetRxBuffer.append(bytes);

    const int maxBufferSize = qMax(frameSize * 8, header.size() + frameSize);
    if (m_packetRxBuffer.size() > maxBufferSize) {
        m_packetRxBuffer = m_packetRxBuffer.right(maxBufferSize);
    }

    while (true) {
        const int headerIndex = m_packetRxBuffer.indexOf(header);
        if (headerIndex < 0) {
            const int keepBytes = qMax(0, header.size() - 1);
            if (m_packetRxBuffer.size() > keepBytes) {
                m_packetRxBuffer = keepBytes > 0 ? m_packetRxBuffer.right(keepBytes) : QByteArray();
            }
            return;
        }

        if (headerIndex > 0) {
            m_packetRxBuffer.remove(0, headerIndex);
        }

        if (m_packetRxBuffer.size() < frameSize) {
            return;
        }

        const QByteArray frame = m_packetRxBuffer.left(frameSize);
        QVector<double> numericValues;
        QStringList displayValues;
        QString errorMessage;
        if (!parsePacketFrame(m_packetSchema, frame, &numericValues, &displayValues, &errorMessage)) {
            logWarn(QStringLiteral("Packet parse failed: %1").arg(errorMessage));
            m_packetRxBuffer.remove(0, 1);
            continue;
        }

        m_packetRxBuffer.remove(0, frameSize);
        ++m_parsedFrameCount;
        emit parsedFrameCountChanged();

        m_packetFieldModel.updateReceivedValues(displayValues);
        updateChart(numericValues);
    }
}

void AppController::updateChart(const QVector<double> &numericValues)
{
    if (m_chartSeriesNames.isEmpty() || numericValues.isEmpty()) {
        return;
    }

    if (m_chartPoints.size() != m_chartSeriesNames.size()) {
        m_chartPoints.resize(m_chartSeriesNames.size());
    }

    const double xValue = static_cast<double>(qMax(0, m_parsedFrameCount - 1));
    int seriesIndex = 0;

    for (int fieldIndex = 0; fieldIndex < m_packetSchema.fields.size() && fieldIndex < numericValues.size(); ++fieldIndex) {
        if (!m_packetSchema.fields.at(fieldIndex).chart) {
            continue;
        }

        if (seriesIndex >= m_chartPoints.size()) {
            break;
        }

        QVector<QPointF> &points = m_chartPoints[seriesIndex];
        points.append(QPointF(xValue, numericValues.at(fieldIndex)));
        while (points.size() > m_chartPointWindow) {
            points.removeFirst();
        }

        emit chartPointAppended(seriesIndex, xValue, numericValues.at(fieldIndex));
        ++seriesIndex;
    }

    recalculateChartRange();
}

void AppController::recalculateChartRange()
{
    if (m_chartPoints.isEmpty()) {
        m_chartXMin = 0.0;
        m_chartXMax = static_cast<double>(m_chartPointWindow);
        m_chartYMin = -1.0;
        m_chartYMax = 1.0;
        emit chartRangeChanged();
        return;
    }

    bool hasPoint = false;
    double latestX = 0.0;
    double minY = std::numeric_limits<double>::max();
    double maxY = std::numeric_limits<double>::lowest();

    for (const QVector<QPointF> &series : std::as_const(m_chartPoints)) {
        for (const QPointF &point : series) {
            hasPoint = true;
            latestX = qMax(latestX, point.x());
            minY = qMin(minY, point.y());
            maxY = qMax(maxY, point.y());
        }
    }

    if (!hasPoint) {
        m_chartXMin = 0.0;
        m_chartXMax = static_cast<double>(m_chartPointWindow);
        m_chartYMin = -1.0;
        m_chartYMax = 1.0;
        emit chartRangeChanged();
        return;
    }

    m_chartXMax = qMax(static_cast<double>(m_chartPointWindow), latestX + 1.0);
    m_chartXMin = qMax(0.0, m_chartXMax - static_cast<double>(m_chartPointWindow));

    const double padding = minY == maxY
        ? qMax(1.0, qAbs(minY) * 0.15 + 0.5)
        : (maxY - minY) * 0.12;
    m_chartYMin = minY - padding;
    m_chartYMax = maxY + padding;
    emit chartRangeChanged();
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

QString AppController::normalizedPacketSchemaPath(const QString &path) const
{
    if (path.trimmed().isEmpty()) {
        return defaultPacketSchemaPath();
    }

    return QFileInfo(path).absoluteFilePath();
}

QString AppController::defaultPacketSchemaPath() const
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates{
        QFileInfo(appDir + QStringLiteral("/../share/gnu_jcom/examples/linear_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../share/gnu_jcom/examples/sine_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../examples/linear_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../examples/sine_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/examples/linear_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/examples/sine_demo_schema.json")).absoluteFilePath(),
    };

    for (const QString &candidate : candidates) {
        if (QFileInfo::exists(candidate)) {
            return candidate;
        }
    }

    return candidates.first();
}
