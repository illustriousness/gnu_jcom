#pragma once

#include "logmodel.h"
#include "packetfieldmodel.h"
#include "packetschema.h"
#include "sendlistmodel.h"

#include <QFileInfo>
#include <QJsonObject>
#include <QObject>
#include <QPointF>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QTimer>

class AppController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
    Q_PROPERTY(QString selectedPort READ selectedPort WRITE setSelectedPort NOTIFY selectedPortChanged)
    Q_PROPERTY(int baudRate READ baudRate WRITE setBaudRate NOTIFY baudRateChanged)
    Q_PROPERTY(int dataBits READ dataBits WRITE setDataBits NOTIFY dataBitsChanged)
    Q_PROPERTY(int stopBits READ stopBits WRITE setStopBits NOTIFY stopBitsChanged)
    Q_PROPERTY(int parity READ parity WRITE setParity NOTIFY parityChanged)
    Q_PROPERTY(int flowControl READ flowControl WRITE setFlowControl NOTIFY flowControlChanged)
    Q_PROPERTY(bool autoReconnect READ autoReconnect WRITE setAutoReconnect NOTIFY autoReconnectChanged)
    Q_PROPERTY(bool portOpen READ portOpen NOTIFY portOpenChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(bool hexDisplay READ hexDisplay WRITE setHexDisplay NOTIFY hexDisplayChanged)
    Q_PROPERTY(int composeMode READ composeMode WRITE setComposeMode NOTIFY composeModeChanged)
    Q_PROPERTY(int lineEnding READ lineEnding WRITE setLineEnding NOTIFY lineEndingChanged)
    Q_PROPERTY(bool periodicActive READ periodicActive NOTIFY periodicActiveChanged)
    Q_PROPERTY(int periodicIntervalMs READ periodicIntervalMs WRITE setPeriodicIntervalMs NOTIFY periodicIntervalMsChanged)
    Q_PROPERTY(QString workspacePath READ workspacePath WRITE setWorkspacePath NOTIFY workspacePathChanged)
    Q_PROPERTY(QString defaultWorkspacePath READ defaultWorkspacePath CONSTANT)
    Q_PROPERTY(QString logExportPath READ logExportPath WRITE setLogExportPath NOTIFY logExportPathChanged)
    Q_PROPERTY(QString defaultLogExportPath READ defaultLogExportPath CONSTANT)
    Q_PROPERTY(QString packetSchemaPath READ packetSchemaPath WRITE setPacketSchemaPath NOTIFY packetSchemaPathChanged)
    Q_PROPERTY(QString packetSchemaName READ packetSchemaName WRITE setPacketSchemaName NOTIFY packetSchemaNameChanged)
    Q_PROPERTY(QString packetHeaderText READ packetHeaderText WRITE setPacketHeaderText NOTIFY packetHeaderTextChanged)
    Q_PROPERTY(QString packetFooterText READ packetFooterText WRITE setPacketFooterText NOTIFY packetFooterTextChanged)
    Q_PROPERTY(QString packetChecksum READ packetChecksum WRITE setPacketChecksum NOTIFY packetChecksumChanged)
    Q_PROPERTY(bool packetSchemaLoaded READ packetSchemaLoaded NOTIFY packetSchemaLoadedChanged)
    Q_PROPERTY(int parsedFrameCount READ parsedFrameCount NOTIFY parsedFrameCountChanged)
    Q_PROPERTY(QStringList chartSeriesNames READ chartSeriesNames NOTIFY chartSeriesNamesChanged)
    Q_PROPERTY(double chartXMin READ chartXMin NOTIFY chartRangeChanged)
    Q_PROPERTY(double chartXMax READ chartXMax NOTIFY chartRangeChanged)
    Q_PROPERTY(double chartYMin READ chartYMin NOTIFY chartRangeChanged)
    Q_PROPERTY(double chartYMax READ chartYMax NOTIFY chartRangeChanged)
    Q_PROPERTY(LogModel *logModel READ logModel CONSTANT)
    Q_PROPERTY(SendListModel *sendListModel READ sendListModel CONSTANT)
    Q_PROPERTY(PacketFieldModel *packetFieldModel READ packetFieldModel CONSTANT)

public:
    explicit AppController(QObject *parent = nullptr);

    QStringList availablePorts() const;
    QString selectedPort() const;
    int baudRate() const;
    int dataBits() const;
    int stopBits() const;
    int parity() const;
    int flowControl() const;
    bool autoReconnect() const;
    bool portOpen() const;
    QString statusMessage() const;
    QString lastError() const;
    bool hexDisplay() const;
    int composeMode() const;
    int lineEnding() const;
    bool periodicActive() const;
    int periodicIntervalMs() const;
    QString workspacePath() const;
    QString defaultWorkspacePath() const;
    QString logExportPath() const;
    QString defaultLogExportPath() const;
    QString packetSchemaPath() const;
    QString packetSchemaName() const;
    QString packetHeaderText() const;
    QString packetFooterText() const;
    QString packetChecksum() const;
    bool packetSchemaLoaded() const;
    int parsedFrameCount() const;
    QStringList chartSeriesNames() const;
    double chartXMin() const;
    double chartXMax() const;
    double chartYMin() const;
    double chartYMax() const;
    LogModel *logModel();
    SendListModel *sendListModel();
    PacketFieldModel *packetFieldModel();

    void setSelectedPort(const QString &portName);
    void setBaudRate(int baudRate);
    void setDataBits(int dataBits);
    void setStopBits(int stopBits);
    void setParity(int parity);
    void setFlowControl(int flowControl);
    void setAutoReconnect(bool autoReconnect);
    void setHexDisplay(bool hexDisplay);
    void setComposeMode(int composeMode);
    void setLineEnding(int lineEnding);
    void setPeriodicIntervalMs(int intervalMs);
    void setWorkspacePath(const QString &path);
    void setLogExportPath(const QString &path);
    void setPacketSchemaPath(const QString &path);
    void setPacketSchemaName(const QString &name);
    void setPacketHeaderText(const QString &text);
    void setPacketFooterText(const QString &text);
    void setPacketChecksum(const QString &checksum);

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE bool openPort();
    Q_INVOKABLE void closePort();
    Q_INVOKABLE bool sendPayload(const QString &payload, int mode);
    Q_INVOKABLE bool startPeriodicSend(const QString &payload, int mode, int intervalMs);
    Q_INVOKABLE void stopPeriodicSend();
    Q_INVOKABLE bool addSendItem(const QString &name, const QString &payload, int mode);
    Q_INVOKABLE bool updateSendItem(int index, const QString &name, const QString &payload, int mode);
    Q_INVOKABLE void removeSendItem(int index);
    Q_INVOKABLE QVariantMap sendItemAt(int index) const;
    Q_INVOKABLE bool sendSavedItem(int index);
    Q_INVOKABLE bool saveWorkspace(const QString &path = QString());
    Q_INVOKABLE bool loadWorkspace(const QString &path = QString());
    Q_INVOKABLE bool saveLog(const QString &path = QString());
    Q_INVOKABLE QString convertPayloadForMode(const QString &payload, int fromMode, int toMode);
    Q_INVOKABLE bool loadPacketSchema(const QString &path = QString());
    Q_INVOKABLE bool savePacketSchema(const QString &path = QString());
    Q_INVOKABLE bool applyPacketDefinition();
    Q_INVOKABLE QString buildPacketPreview();
    Q_INVOKABLE bool sendPacket();
    Q_INVOKABLE void clearLogs();
    Q_INVOKABLE void clearLastError();

signals:
    void availablePortsChanged();
    void selectedPortChanged();
    void baudRateChanged();
    void dataBitsChanged();
    void stopBitsChanged();
    void parityChanged();
    void flowControlChanged();
    void autoReconnectChanged();
    void portOpenChanged();
    void statusMessageChanged();
    void lastErrorChanged();
    void hexDisplayChanged();
    void composeModeChanged();
    void lineEndingChanged();
    void periodicActiveChanged();
    void periodicIntervalMsChanged();
    void workspacePathChanged();
    void logExportPathChanged();
    void packetSchemaPathChanged();
    void packetSchemaNameChanged();
    void packetHeaderTextChanged();
    void packetFooterTextChanged();
    void packetChecksumChanged();
    void packetSchemaLoadedChanged();
    void parsedFrameCountChanged();
    void chartSeriesNamesChanged();
    void chartRangeChanged();
    void chartReset();
    void chartPointAppended(int seriesIndex, double x, double y);

private:
    void setStatusMessage(const QString &message);
    void setLastError(const QString &message);
    void logInfo(const QString &message);
    void logWarn(const QString &message);
    void logError(const QString &message);
    void logTransfer(const QString &kind, const QByteArray &bytes);

    void configureSerialPort();
    bool openPortInternal(bool autoReconnectAttempt);
    void handlePortError(QSerialPort::SerialPortError error);
    void handleReadyRead();
    void scanPorts();
    void triggerReconnect();
    void stopPeriodicSendInternal(bool logEvent);
    void resetPacketRuntimeState();
    void processPacketBytes(const QByteArray &bytes);
    void updateChart(const QVector<double> &numericValues);
    void recalculateChartRange();

    QSerialPortInfo portInfoForName(const QString &portName) const;
    QString permissionHint(const QSerialPortInfo &portInfo) const;

    QSerialPort::DataBits toDataBitsEnum() const;
    QSerialPort::StopBits toStopBitsEnum() const;
    QSerialPort::Parity toParityEnum() const;
    QSerialPort::FlowControl toFlowControlEnum() const;

    QString normalizedWorkspacePath(const QString &path) const;
    QString normalizedLogPath(const QString &path) const;
    QString normalizedPacketSchemaPath(const QString &path) const;
    QString defaultPacketSchemaPath() const;
    bool sendBytesDirect(const QByteArray &bytes, const QString &kindLabel, const QString &statusLabel);
    bool buildEditorPacketSchema(PacketSchema *schema, QString *errorMessage = nullptr) const;
    void setPacketRuntimeSchema(const PacketSchema &schema, bool logEvent);
    void loadPacketEditorFromJsonObject(const QJsonObject &object);
    QJsonObject packetEditorToJsonObject() const;

    QStringList m_availablePorts;
    QList<QSerialPortInfo> m_portInfos;
    QSerialPort m_serialPort;
    QTimer m_portScanTimer;
    QTimer m_reconnectTimer;
    QTimer m_periodicSendTimer;
    QString m_selectedPort;
    int m_baudRate = 115200;
    int m_dataBits = 8;
    int m_stopBits = 1;
    int m_parity = 0;
    int m_flowControl = 0;
    bool m_autoReconnect = true;
    QString m_statusMessage = QStringLiteral("Idle");
    QString m_lastError;
    bool m_hexDisplay = false;
    int m_composeMode = 0;
    int m_lineEnding = 0;
    int m_periodicIntervalMs = 1000;
    QString m_periodicPayload;
    int m_periodicMode = 0;
    QString m_workspacePath;
    QString m_logExportPath;
    QString m_packetSchemaPath;
    QString m_packetSchemaName = QStringLiteral("linear_demo");
    QString m_packetHeaderText = QStringLiteral("AA 55");
    QString m_packetFooterText;
    QString m_packetChecksum = QStringLiteral("sum8");
    int m_packetChecksumStart = 0;
    PacketSchema m_packetSchema;
    QByteArray m_packetRxBuffer;
    bool m_reconnectPending = false;
    bool m_autoReconnectAttemptInFlight = false;
    QString m_lastReconnectError;
    int m_parsedFrameCount = 0;
    QStringList m_chartSeriesNames;
    QVector<QVector<QPointF>> m_chartPoints;
    int m_chartPointWindow = 256;
    double m_chartXMin = 0.0;
    double m_chartXMax = 256.0;
    double m_chartYMin = -1.0;
    double m_chartYMax = 1.0;
    LogModel m_logModel;
    SendListModel m_sendListModel;
    PacketFieldModel m_packetFieldModel;
};
