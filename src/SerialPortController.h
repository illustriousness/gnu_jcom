#pragma once

#include <QByteArray>
#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QTimer>
#include <QVector>

#include "gnu_soc_proto.h"

class McuReportModel;

class SerialPortController final : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)
    Q_PROPERTY(QString selectedPort READ selectedPort WRITE setSelectedPort NOTIFY selectedPortChanged)
    Q_PROPERTY(int baudRate READ baudRate WRITE setBaudRate NOTIFY baudRateChanged)
    Q_PROPERTY(int dataBits READ dataBits WRITE setDataBits NOTIFY dataBitsChanged)
    Q_PROPERTY(int stopBits READ stopBits WRITE setStopBits NOTIFY stopBitsChanged)
    Q_PROPERTY(int parity READ parity WRITE setParity NOTIFY parityChanged)
    Q_PROPERTY(int flowControl READ flowControl WRITE setFlowControl NOTIFY flowControlChanged)
    Q_PROPERTY(bool portOpen READ portOpen NOTIFY portOpenChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY lastErrorChanged)
    Q_PROPERTY(quint64 rxBytes READ rxBytes NOTIFY statsChanged)
    Q_PROPERTY(quint64 txBytes READ txBytes NOTIFY statsChanged)
    Q_PROPERTY(quint64 parsedFrames READ parsedFrames NOTIFY statsChanged)
    Q_PROPERTY(quint64 txFrames READ txFrames NOTIFY statsChanged)
    Q_PROPERTY(quint64 badFrames READ badFrames NOTIFY statsChanged)
    Q_PROPERTY(bool terminalMode READ terminalMode NOTIFY terminalModeChanged)
    Q_PROPERTY(QString terminalBuffer READ terminalBuffer NOTIFY terminalBufferChanged)
    Q_PROPERTY(QString terminalHtml READ terminalHtml NOTIFY terminalBufferChanged)
    Q_PROPERTY(int terminalCursorPosition READ terminalCursorPosition NOTIFY terminalBufferChanged)
    Q_PROPERTY(int controlV READ controlV WRITE setControlV NOTIFY controlConfigChanged)
    Q_PROPERTY(int controlW READ controlW WRITE setControlW NOTIFY controlConfigChanged)
    Q_PROPERTY(int controlPeriodMs READ controlPeriodMs WRITE setControlPeriodMs NOTIFY controlConfigChanged)
    Q_PROPERTY(bool timedVwRunning READ timedVwRunning NOTIFY timedVwRunningChanged)

public:
    explicit SerialPortController(McuReportModel *reportModel, QObject *parent = nullptr);

    QStringList availablePorts() const { return m_availablePorts; }
    QString selectedPort() const { return m_selectedPort; }
    int baudRate() const { return m_baudRate; }
    int dataBits() const { return m_dataBits; }
    int stopBits() const { return m_stopBits; }
    int parity() const { return m_parity; }
    int flowControl() const { return m_flowControl; }
    bool portOpen() const { return m_serialPort.isOpen(); }
    QString statusMessage() const { return m_statusMessage; }
    QString lastError() const { return m_lastError; }
    quint64 rxBytes() const { return m_rxBytes; }
    quint64 txBytes() const { return m_txBytes; }
    quint64 parsedFrames() const { return m_parsedFrames; }
    quint64 txFrames() const { return m_txFrames; }
    quint64 badFrames() const { return m_badFrames; }
    bool terminalMode() const { return m_terminalMode; }
    QString terminalBuffer() const { return m_terminalBuffer; }
    QString terminalHtml() const;
    int terminalCursorPosition() const { return static_cast<int>(qBound<qsizetype>(0, m_terminalCursor, m_terminalBuffer.size())); }
    int controlV() const { return m_controlV; }
    int controlW() const { return m_controlW; }
    int controlPeriodMs() const { return m_controlPeriodMs; }
    bool timedVwRunning() const { return m_vwControlTimer.isActive(); }

    void setSelectedPort(const QString &portName);
    void setBaudRate(int baudRate);
    void setDataBits(int dataBits);
    void setStopBits(int stopBits);
    void setParity(int parity);
    void setFlowControl(int flowControl);
    void setControlV(int controlV);
    void setControlW(int controlW);
    void setControlPeriodMs(int periodMs);

    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE bool openPort();
    Q_INVOKABLE void closePort();
    Q_INVOKABLE void clearStats();
    Q_INVOKABLE void clearLastError();
    Q_INVOKABLE bool sendVwControl(int controlV, int controlW);
    Q_INVOKABLE bool sendCurrentTimestamp();
    Q_INVOKABLE bool sendMotorConfig(int motorId, int payload);
    Q_INVOKABLE bool sendLedConfig(int mode, int ledId, int red, int green, int blue, int intervalMs);
    Q_INVOKABLE bool sendPowerControl(int action);
    Q_INVOKABLE bool sendBuzzerControl(int buzzerId, int repeatCount, int onTimeMs, int offTimeMs);
    Q_INVOKABLE bool activateTerminal();
    Q_INVOKABLE bool sendTerminalText(const QString &text);
    Q_INVOKABLE bool sendClipboardText();
    Q_INVOKABLE void copyTextToClipboard(const QString &text);
    Q_INVOKABLE void clearTerminalBuffer();
    Q_INVOKABLE bool startTimedVwControl();
    Q_INVOKABLE void stopTimedVwControl();

signals:
    void availablePortsChanged();
    void selectedPortChanged();
    void baudRateChanged();
    void dataBitsChanged();
    void stopBitsChanged();
    void parityChanged();
    void flowControlChanged();
    void portOpenChanged();
    void statusMessageChanged();
    void lastErrorChanged();
    void statsChanged();
    void controlConfigChanged();
    void timedVwRunningChanged();
    void terminalModeChanged();
    void terminalBufferChanged();

private:
    enum class TerminalParserState {
        Ground,
        Escape,
        Csi,
    };
    static constexpr quint32 kTerminalDefaultStyle = 0x00020100U;

    void configureSerialPort();
    void handleReadyRead();
    void handlePortError(QSerialPort::SerialPortError error);
    void processBytes(const QByteArray &bytes);
    void appendTerminalBytes(const QByteArray &bytes);
    void appendTerminalLine(const QString &line);
    void setTerminalMode(bool enabled);
    void resetTerminalRenderer();
    void renderTerminalBytes(const QByteArray &bytes);
    void putTerminalText(const QString &text);
    void putTerminalChar(QChar ch);
    void handleTerminalControl(char ch);
    void finishTerminalCsi(char finalByte);
    void applyTerminalSgr(const QVector<int> &params);
    void insertTerminalText(qsizetype position, const QString &text, quint32 style);
    void removeTerminalRange(qsizetype position, qsizetype count);
    void truncateTerminalBuffer(qsizetype size);
    qsizetype terminalLineStart(qsizetype cursor) const;
    qsizetype terminalLineEnd(qsizetype cursor) const;
    qsizetype terminalCursorForRowColumn(int row, int column);
    void moveTerminalCursorVertical(int delta);
    void trimTerminalBuffer();
    bool sendCurrentVwControl();
    bool writeProtocolFrame(uint8_t cmdId, const uint8_t *payload, uint8_t payloadLen, const QString &frameName);
    void setStatusMessage(const QString &message);
    void setLastError(const QString &message);
    void scanPorts();
    bool validateInt16(const QString &name, int value);
    bool validateUint8(const QString &name, int value);
    bool validateUint16(const QString &name, int value);

    QSerialPortInfo portInfoForName(const QString &portName) const;
    QString permissionHint(const QSerialPortInfo &portInfo) const;

    QSerialPort::DataBits toDataBitsEnum() const;
    QSerialPort::StopBits toStopBitsEnum() const;
    QSerialPort::Parity toParityEnum() const;
    QSerialPort::FlowControl toFlowControlEnum() const;

    McuReportModel *m_reportModel = nullptr;
    QSerialPort m_serialPort;
    QTimer m_portScanTimer;
    QStringList m_availablePorts;
    QList<QSerialPortInfo> m_portInfos;
    QString m_selectedPort;
    int m_baudRate = 115200;
    int m_dataBits = 8;
    int m_stopBits = 1;
    int m_parity = 0;
    int m_flowControl = 0;
    QString m_statusMessage = QStringLiteral("串口未连接");
    QString m_lastError;
    quint64 m_rxBytes = 0;
    quint64 m_txBytes = 0;
    quint64 m_parsedFrames = 0;
    quint64 m_txFrames = 0;
    quint64 m_badFrames = 0;
    bool m_terminalMode = false;
    bool m_terminalOutputPrevWasCr = false;
    QString m_terminalBuffer;
    QVector<quint32> m_terminalStyles;
    quint32 m_terminalCurrentStyle = kTerminalDefaultStyle;
    qsizetype m_terminalCursor = 0;
    qsizetype m_terminalSavedCursor = 0;
    QByteArray m_terminalEscapeBuffer;
    TerminalParserState m_terminalParserState = TerminalParserState::Ground;
    int m_controlV = 0;
    int m_controlW = 0;
    int m_controlPeriodMs = 100;
    gnu_soc_proto_parser_t m_parser{};
    QTimer m_vwControlTimer;
};
