#pragma once

#include "logmodel.h"
#include "sendlistmodel.h"

#include <QFileInfo>
#include <QObject>
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
    Q_PROPERTY(LogModel *logModel READ logModel CONSTANT)
    Q_PROPERTY(SendListModel *sendListModel READ sendListModel CONSTANT)

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
    LogModel *logModel();
    SendListModel *sendListModel();

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

    QSerialPortInfo portInfoForName(const QString &portName) const;
    QString permissionHint(const QSerialPortInfo &portInfo) const;

    QSerialPort::DataBits toDataBitsEnum() const;
    QSerialPort::StopBits toStopBitsEnum() const;
    QSerialPort::Parity toParityEnum() const;
    QSerialPort::FlowControl toFlowControlEnum() const;

    QString normalizedWorkspacePath(const QString &path) const;
    QString normalizedLogPath(const QString &path) const;

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
    bool m_reconnectPending = false;
    bool m_autoReconnectAttemptInFlight = false;
    QString m_lastReconnectError;
    LogModel m_logModel;
    SendListModel m_sendListModel;
};
