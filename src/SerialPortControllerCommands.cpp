#include "SerialPortController.h"

#include <QDateTime>

#include <limits>

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

bool SerialPortController::sendMotorConfig(int motorId, int payload)
{
    if (!validateUint8(QStringLiteral("motor_id"), motorId)) {
        return false;
    }

    const gnu_soc_proto_motor_cfg_t config{
        static_cast<uint8_t>(motorId),
        static_cast<int32_t>(payload),
    };

    if (!writeProtocolFrame(static_cast<uint8_t>(GNU_SOC_PROTO_CMD_CTRL_MOTOR),
                            reinterpret_cast<const uint8_t *>(&config),
                            static_cast<uint8_t>(sizeof(config)),
                            QStringLiteral("电机配置"))) {
        return false;
    }

    setStatusMessage(QStringLiteral("已下发电机配置：motor_id=%1 payload=%2").arg(motorId).arg(payload));
    return true;
}

bool SerialPortController::sendLedConfig(int mode, int ledId, int red, int green, int blue, int intervalMs)
{
    if (mode < GNU_SOC_PROTO_LED_BLINK || mode > GNU_SOC_PROTO_LED_BREATHE) {
        setLastError(QStringLiteral("LED mode 只支持 0(闪烁)、1(流水)、2(呼吸)。"));
        return false;
    }
    if (ledId < GNU_SOC_PROTO_LED_ID_STRIP || ledId > GNU_SOC_PROTO_LED_ID_LED0) {
        setLastError(QStringLiteral("LED id 只支持 0(灯带) 或 1(LED0)。"));
        return false;
    }
    if (!validateUint8(QStringLiteral("R"), red) ||
        !validateUint8(QStringLiteral("G"), green) ||
        !validateUint8(QStringLiteral("B"), blue) ||
        !validateUint16(QStringLiteral("interval"), intervalMs)) {
        return false;
    }

    const gnu_soc_proto_led_cfg_t config{
        gnu_soc_proto_led_make_mode_and_id(static_cast<uint8_t>(mode), static_cast<uint8_t>(ledId)),
        static_cast<uint8_t>(red),
        static_cast<uint8_t>(green),
        static_cast<uint8_t>(blue),
        static_cast<uint16_t>(intervalMs),
    };

    if (!writeProtocolFrame(static_cast<uint8_t>(GNU_SOC_PROTO_CMD_CTRL_LED),
                            reinterpret_cast<const uint8_t *>(&config),
                            static_cast<uint8_t>(sizeof(config)),
                            QStringLiteral("LED 配置"))) {
        return false;
    }

    setStatusMessage(QStringLiteral("已下发 LED 配置：mode=%1 id=%2 rgb=(%3,%4,%5) interval=%6 ms")
                         .arg(mode)
                         .arg(ledId)
                         .arg(red)
                         .arg(green)
                         .arg(blue)
                         .arg(intervalMs));
    return true;
}

bool SerialPortController::sendPowerControl(int action)
{
    if (action != GNU_SOC_PROTO_POWER_ACTION_SHUTDOWN && action != GNU_SOC_PROTO_POWER_ACTION_SLEEP) {
        setLastError(QStringLiteral("电源 action 只支持 0(关机) 或 1(休眠)。"));
        return false;
    }

    const gnu_soc_proto_power_cfg_t config{
        static_cast<uint8_t>(action),
    };

    if (!writeProtocolFrame(static_cast<uint8_t>(GNU_SOC_PROTO_CMD_CTRL_POWER),
                            reinterpret_cast<const uint8_t *>(&config),
                            static_cast<uint8_t>(sizeof(config)),
                            QStringLiteral("电源控制"))) {
        return false;
    }

    setStatusMessage(action == GNU_SOC_PROTO_POWER_ACTION_SLEEP
                         ? QStringLiteral("已下发电源控制：休眠")
                         : QStringLiteral("已下发电源控制：关机"));
    return true;
}

bool SerialPortController::sendBuzzerControl(int buzzerId, int repeatCount, int onTimeMs, int offTimeMs)
{
    if (!validateUint8(QStringLiteral("buzzer_id"), buzzerId) ||
        !validateUint8(QStringLiteral("repeat_cnt"), repeatCount) ||
        !validateUint16(QStringLiteral("on_time_ms"), onTimeMs) ||
        !validateUint16(QStringLiteral("off_time_ms"), offTimeMs)) {
        return false;
    }

    const gnu_soc_proto_buzzer_ctrl_t control{
        static_cast<uint8_t>(buzzerId),
        static_cast<uint8_t>(repeatCount),
        static_cast<uint16_t>(onTimeMs),
        static_cast<uint16_t>(offTimeMs),
    };

    if (!writeProtocolFrame(static_cast<uint8_t>(GNU_SOC_PROTO_CMD_CTRL_BUZZER),
                            reinterpret_cast<const uint8_t *>(&control),
                            static_cast<uint8_t>(sizeof(control)),
                            QStringLiteral("蜂鸣器控制"))) {
        return false;
    }

    setStatusMessage(QStringLiteral("已下发蜂鸣器控制：id=%1 repeat=%2 on=%3 ms off=%4 ms")
                         .arg(buzzerId)
                         .arg(repeatCount)
                         .arg(onTimeMs)
                         .arg(offTimeMs));
    return true;
}
