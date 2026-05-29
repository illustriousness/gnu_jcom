#include "SerialPortController.h"

#include "McuReportModel.h"

#include <QByteArray>
#include <QDateTime>
#include <QFileInfo>
#include <QIODevice>

#include <cstring>
#include <limits>

SerialPortController::SerialPortController(McuReportModel *reportModel, QObject *parent)
    : QObject(parent)
    , m_reportModel(reportModel)
{
    gnu_soc_proto_parser_init(&m_parser);

    connect(&m_serialPort, &QSerialPort::readyRead, this, &SerialPortController::handleReadyRead);
    connect(&m_serialPort, &QSerialPort::errorOccurred, this, &SerialPortController::handlePortError);
    connect(&m_portScanTimer, &QTimer::timeout, this, &SerialPortController::scanPorts);
    connect(&m_vwControlTimer, &QTimer::timeout, this, &SerialPortController::sendCurrentVwControl);

    scanPorts();
    m_portScanTimer.start(1000);
    m_vwControlTimer.setTimerType(Qt::PreciseTimer);
}

void SerialPortController::setSelectedPort(const QString &portName)
{
    const QString normalized = portName.trimmed();
    if (m_selectedPort == normalized) {
        return;
    }

    m_selectedPort = normalized;
    emit selectedPortChanged();
}

void SerialPortController::setBaudRate(int baudRate)
{
    if (baudRate <= 0) {
        setLastError(QStringLiteral("波特率必须大于 0。"));
        return;
    }

    if (m_baudRate == baudRate) {
        return;
    }

    m_baudRate = baudRate;
    emit baudRateChanged();
}

void SerialPortController::setDataBits(int dataBits)
{
    if (dataBits < 5 || dataBits > 8) {
        setLastError(QStringLiteral("数据位只支持 5、6、7、8。"));
        return;
    }

    if (m_dataBits == dataBits) {
        return;
    }

    m_dataBits = dataBits;
    emit dataBitsChanged();
}

void SerialPortController::setStopBits(int stopBits)
{
    if (stopBits != 1 && stopBits != 2) {
        setLastError(QStringLiteral("停止位只支持 1 或 2。"));
        return;
    }

    if (m_stopBits == stopBits) {
        return;
    }

    m_stopBits = stopBits;
    emit stopBitsChanged();
}

void SerialPortController::setParity(int parity)
{
    if (parity < 0 || parity > 4) {
        setLastError(QStringLiteral("校验位参数无效。"));
        return;
    }

    if (m_parity == parity) {
        return;
    }

    m_parity = parity;
    emit parityChanged();
}

void SerialPortController::setFlowControl(int flowControl)
{
    if (flowControl < 0 || flowControl > 2) {
        setLastError(QStringLiteral("流控参数无效。"));
        return;
    }

    if (m_flowControl == flowControl) {
        return;
    }

    m_flowControl = flowControl;
    emit flowControlChanged();
}

void SerialPortController::setControlV(int controlV)
{
    if (!validateInt16(QStringLiteral("v"), controlV)) {
        return;
    }

    if (m_controlV == controlV) {
        return;
    }

    m_controlV = controlV;
    emit controlConfigChanged();
}

void SerialPortController::setControlW(int controlW)
{
    if (!validateInt16(QStringLiteral("w"), controlW)) {
        return;
    }

    if (m_controlW == controlW) {
        return;
    }

    m_controlW = controlW;
    emit controlConfigChanged();
}

void SerialPortController::setControlPeriodMs(int periodMs)
{
    if (periodMs <= 0) {
        setLastError(QStringLiteral("VW 控制周期必须大于 0 ms。"));
        return;
    }

    if (m_controlPeriodMs == periodMs) {
        return;
    }

    m_controlPeriodMs = periodMs;
    if (m_vwControlTimer.isActive()) {
        m_vwControlTimer.setInterval(m_controlPeriodMs);
    }
    emit controlConfigChanged();
}

void SerialPortController::refreshPorts()
{
    scanPorts();
}

bool SerialPortController::openPort()
{
    if (m_selectedPort.isEmpty()) {
        scanPorts();
    }

    if (m_selectedPort.isEmpty()) {
        setLastError(QStringLiteral("没有可用串口，请插入设备后刷新。"));
        return false;
    }

    if (m_serialPort.isOpen()) {
        if (m_serialPort.portName() == m_selectedPort) {
            return true;
        }

        stopTimedVwControl();
        m_serialPort.close();
        emit portOpenChanged();
    }

    configureSerialPort();
    gnu_soc_proto_parser_init(&m_parser);

    if (!m_serialPort.open(QIODevice::ReadWrite)) {
        QString message = QStringLiteral("打开 %1 失败：%2").arg(m_selectedPort, m_serialPort.errorString());
        if (m_serialPort.error() == QSerialPort::PermissionError) {
            const QString hint = permissionHint(portInfoForName(m_selectedPort));
            if (!hint.isEmpty()) {
                message += QStringLiteral(" %1").arg(hint);
            }
        }
        setLastError(message);
        return false;
    }

    emit portOpenChanged();
    clearLastError();
    setStatusMessage(QStringLiteral("已连接 %1 @ %2").arg(m_selectedPort).arg(m_baudRate));
    return true;
}

void SerialPortController::closePort()
{
    if (!m_serialPort.isOpen()) {
        setStatusMessage(QStringLiteral("串口未连接"));
        return;
    }

    stopTimedVwControl();
    m_serialPort.close();
    gnu_soc_proto_parser_init(&m_parser);
    emit portOpenChanged();
    setStatusMessage(QStringLiteral("已断开 %1").arg(m_selectedPort));
}

void SerialPortController::clearStats()
{
    if (m_rxBytes == 0 && m_txBytes == 0 && m_parsedFrames == 0 && m_txFrames == 0 && m_badFrames == 0) {
        return;
    }

    m_rxBytes = 0;
    m_txBytes = 0;
    m_parsedFrames = 0;
    m_txFrames = 0;
    m_badFrames = 0;
    emit statsChanged();
}

bool SerialPortController::sendVwControl(int controlV, int controlW)
{
    if (!validateInt16(QStringLiteral("v"), controlV) || !validateInt16(QStringLiteral("w"), controlW)) {
        return false;
    }

    m_controlV = controlV;
    m_controlW = controlW;
    emit controlConfigChanged();
    return sendCurrentVwControl();
}

bool SerialPortController::sendCurrentTimestamp()
{
    const qint64 currentSecs = QDateTime::currentSecsSinceEpoch();
    if (currentSecs < 0 || static_cast<quint64>(currentSecs) > std::numeric_limits<uint32_t>::max()) {
        setLastError(QStringLiteral("系统时间戳超出 uint32 范围，无法下发。"));
        return false;
    }

    const gnu_soc_proto_time_cfg_t timeConfig{
        static_cast<uint32_t>(currentSecs),
    };

    if (!writeProtocolFrame(static_cast<uint8_t>(GNU_SOC_PROTO_CMD_SET_TIME),
                            reinterpret_cast<const uint8_t *>(&timeConfig),
                            static_cast<uint8_t>(sizeof(timeConfig)),
                            QStringLiteral("系统时间"))) {
        return false;
    }

    setStatusMessage(QStringLiteral("已下发系统时间：%1").arg(timeConfig.timestamp));
    return true;
}

bool SerialPortController::startTimedVwControl()
{
    if (!m_serialPort.isOpen()) {
        setLastError(QStringLiteral("请先打开串口，再启动 VW 定时下发。"));
        return false;
    }

    if (m_controlPeriodMs <= 0) {
        setLastError(QStringLiteral("VW 控制周期必须大于 0 ms。"));
        return false;
    }

    if (!sendCurrentVwControl()) {
        return false;
    }

    const bool wasRunning = m_vwControlTimer.isActive();
    m_vwControlTimer.start(m_controlPeriodMs);
    if (!wasRunning) {
        emit timedVwRunningChanged();
    }
    setStatusMessage(QStringLiteral("VW 定时下发中：v=%1 w=%2 周期=%3 ms")
                         .arg(m_controlV)
                         .arg(m_controlW)
                         .arg(m_controlPeriodMs));
    return true;
}

void SerialPortController::stopTimedVwControl()
{
    if (!m_vwControlTimer.isActive()) {
        return;
    }

    m_vwControlTimer.stop();
    emit timedVwRunningChanged();
    setStatusMessage(QStringLiteral("VW 定时下发已停止"));
}

void SerialPortController::clearLastError()
{
    if (m_lastError.isEmpty()) {
        return;
    }

    m_lastError.clear();
    emit lastErrorChanged();
}

void SerialPortController::configureSerialPort()
{
    m_serialPort.setPortName(m_selectedPort);
    m_serialPort.setBaudRate(m_baudRate);
    m_serialPort.setDataBits(toDataBitsEnum());
    m_serialPort.setStopBits(toStopBitsEnum());
    m_serialPort.setParity(toParityEnum());
    m_serialPort.setFlowControl(toFlowControlEnum());
}

void SerialPortController::handleReadyRead()
{
    const QByteArray bytes = m_serialPort.readAll();
    if (bytes.isEmpty()) {
        return;
    }

    m_rxBytes += static_cast<quint64>(bytes.size());
    processBytes(bytes);
    emit statsChanged();
    setStatusMessage(QStringLiteral("接收 %1 字节，已解析 %2 帧")
                         .arg(bytes.size())
                         .arg(m_parsedFrames));
}

void SerialPortController::handlePortError(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError || error == QSerialPort::TimeoutError) {
        return;
    }

    switch (error) {
    case QSerialPort::ResourceError:
    case QSerialPort::DeviceNotFoundError:
    case QSerialPort::ReadError:
        stopTimedVwControl();
        if (m_serialPort.isOpen()) {
            m_serialPort.close();
            emit portOpenChanged();
        }
        setLastError(QStringLiteral("串口连接丢失：%1").arg(m_serialPort.errorString()));
        break;
    case QSerialPort::PermissionError:
        setLastError(QStringLiteral("串口权限错误：%1").arg(m_serialPort.errorString()));
        break;
    default:
        break;
    }
}

void SerialPortController::processBytes(const QByteArray &bytes)
{
    gnu_soc_proto_packet_t packet{};

    for (const char rawByte : bytes) {
        const auto status = gnu_soc_proto_parser_feed(&m_parser,
                                                      static_cast<uint8_t>(static_cast<unsigned char>(rawByte)),
                                                      &packet);
        if (status == GNU_SOC_PROTO_STATUS_NEED_MORE) {
            continue;
        }

        if (status == GNU_SOC_PROTO_STATUS_BAD_CRC || status == GNU_SOC_PROTO_STATUS_BAD_FRAME) {
            m_badFrames++;
            setLastError(status == GNU_SOC_PROTO_STATUS_BAD_CRC
                             ? QStringLiteral("协议 CRC 校验失败。")
                             : QStringLiteral("协议帧格式错误。"));
            continue;
        }

        if (status != GNU_SOC_PROTO_STATUS_OK) {
            m_badFrames++;
            setLastError(QStringLiteral("协议解析失败，状态码 %1。").arg(static_cast<int>(status)));
            continue;
        }

        if (packet.cmd_id != static_cast<uint8_t>(GNU_SOC_PROTO_CMD_REPORT_MCU_STATE)) {
            continue;
        }

        if (packet.payload_len != sizeof(gnu_soc_proto_mcu_report_t)) {
            m_badFrames++;
            setLastError(QStringLiteral("MCU 上报载荷长度错误：%1，期望 %2。")
                             .arg(packet.payload_len)
                             .arg(sizeof(gnu_soc_proto_mcu_report_t)));
            continue;
        }

        gnu_soc_proto_mcu_report_t report{};
        std::memcpy(&report, packet.payload, sizeof(report));
        if (m_reportModel != nullptr) {
            m_reportModel->update(report);
        }
        m_parsedFrames++;
    }
}

bool SerialPortController::sendCurrentVwControl()
{
    if (!validateInt16(QStringLiteral("v"), m_controlV) || !validateInt16(QStringLiteral("w"), m_controlW)) {
        return false;
    }

    const gnu_soc_proto_soc_ctrl_t control{
        static_cast<int16_t>(m_controlV),
        static_cast<int16_t>(m_controlW),
    };

    if (!writeProtocolFrame(static_cast<uint8_t>(GNU_SOC_PROTO_CMD_REPORT_SOC_CTRL),
                            reinterpret_cast<const uint8_t *>(&control),
                            static_cast<uint8_t>(sizeof(control)),
                            QStringLiteral("VW 控制"))) {
        return false;
    }

    setStatusMessage(QStringLiteral("已下发 VW 控制：v=%1 w=%2").arg(m_controlV).arg(m_controlW));
    return true;
}

bool SerialPortController::writeProtocolFrame(uint8_t cmdId,
                                              const uint8_t *payload,
                                              uint8_t payloadLen,
                                              const QString &frameName)
{
    if (!m_serialPort.isOpen()) {
        setLastError(QStringLiteral("请先打开串口，再下发 %1。").arg(frameName));
        return false;
    }

    uint8_t frame[GNU_SOC_PROTO_FRAME_MAX_LEN]{};
    size_t frameLen = 0;
    const auto status = gnu_soc_proto_build_frame(cmdId,
                                                  payload,
                                                  payloadLen,
                                                  frame,
                                                  sizeof(frame),
                                                  &frameLen);
    if (status != GNU_SOC_PROTO_STATUS_OK) {
        setLastError(QStringLiteral("构造 %1 帧失败，状态码 %2。").arg(frameName).arg(static_cast<int>(status)));
        return false;
    }

    const QByteArray bytes(reinterpret_cast<const char *>(frame), static_cast<qsizetype>(frameLen));
    const qint64 queued = m_serialPort.write(bytes);
    if (queued != bytes.size()) {
        setLastError(QStringLiteral("%1 帧写串口失败：%2").arg(frameName, m_serialPort.errorString()));
        return false;
    }

    m_txBytes += static_cast<quint64>(queued);
    m_txFrames++;
    emit statsChanged();
    return true;
}

void SerialPortController::setStatusMessage(const QString &message)
{
    if (m_statusMessage == message) {
        return;
    }

    m_statusMessage = message;
    emit statusMessageChanged();
}

void SerialPortController::setLastError(const QString &message)
{
    if (message.isEmpty() || m_lastError == message) {
        return;
    }

    m_lastError = message;
    emit lastErrorChanged();
    setStatusMessage(message);
}

void SerialPortController::scanPorts()
{
    const QList<QSerialPortInfo> currentInfos = QSerialPortInfo::availablePorts();
    QStringList currentNames;
    currentNames.reserve(currentInfos.size());

    for (const QSerialPortInfo &info : currentInfos) {
        currentNames.append(info.portName());
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

QSerialPortInfo SerialPortController::portInfoForName(const QString &portName) const
{
    for (const QSerialPortInfo &info : m_portInfos) {
        if (info.portName() == portName || info.systemLocation() == portName) {
            return info;
        }
    }

    return {};
}

QString SerialPortController::permissionHint(const QSerialPortInfo &portInfo) const
{
    if (portInfo.portName().isEmpty()) {
        return {};
    }

    const QFileInfo fileInfo(portInfo.systemLocation());
    QString groupHint = QStringLiteral("dialout,uucp,lock");
    if (!fileInfo.group().isEmpty() && fileInfo.group() != QStringLiteral("root")) {
        groupHint = fileInfo.group();
        if (groupHint != QStringLiteral("dialout") && groupHint != QStringLiteral("uucp") && groupHint != QStringLiteral("lock")) {
            groupHint += QStringLiteral(",dialout,uucp,lock");
        }
    }

    return QStringLiteral("请确认当前用户属于串口设备组（常见为 %1），加入后需重新登录。").arg(groupHint);
}

bool SerialPortController::validateInt16(const QString &name, int value)
{
    if (value < std::numeric_limits<int16_t>::min() || value > std::numeric_limits<int16_t>::max()) {
        setLastError(QStringLiteral("%1 必须在 int16 范围内（-32768 ~ 32767）。").arg(name));
        return false;
    }

    return true;
}

QSerialPort::DataBits SerialPortController::toDataBitsEnum() const
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

QSerialPort::StopBits SerialPortController::toStopBitsEnum() const
{
    return m_stopBits == 2 ? QSerialPort::TwoStop : QSerialPort::OneStop;
}

QSerialPort::Parity SerialPortController::toParityEnum() const
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
    case 0:
    default:
        return QSerialPort::NoParity;
    }
}

QSerialPort::FlowControl SerialPortController::toFlowControlEnum() const
{
    switch (m_flowControl) {
    case 1:
        return QSerialPort::HardwareControl;
    case 2:
        return QSerialPort::SoftwareControl;
    case 0:
    default:
        return QSerialPort::NoFlowControl;
    }
}
