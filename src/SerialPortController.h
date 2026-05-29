#pragma once

#include <QObject>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QStringList>
#include <QTimer>

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

private:
    void configureSerialPort();
    void handleReadyRead();
    void handlePortError(QSerialPort::SerialPortError error);
    void processBytes(const QByteArray &bytes);
    bool sendCurrentVwControl();
    bool writeProtocolFrame(uint8_t cmdId, const uint8_t *payload, uint8_t payloadLen, const QString &frameName);
    void setStatusMessage(const QString &message);
    void setLastError(const QString &message);
    void scanPorts();
    bool validateInt16(const QString &name, int value);

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
    int m_controlV = 0;
    int m_controlW = 0;
    int m_controlPeriodMs = 100;
    gnu_soc_proto_parser_t m_parser{};
    QTimer m_vwControlTimer;
};
