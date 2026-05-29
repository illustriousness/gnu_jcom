#pragma once
#include <QObject>
#include <QTimer>
#include "gnu_soc_proto.h"

// 将 gnu_soc_proto_mcu_report_t 的每个字段暴露为 Q_PROPERTY，
// QML 通过 report.xxx 直接绑定，update() 触发 updated() 信号批量刷新。
class McuReportModel : public QObject
{
    Q_OBJECT

    // 运动
    Q_PROPERTY(int vReal  READ vReal  NOTIFY updated)
    Q_PROPERTY(int wReal  READ wReal  NOTIFY updated)

    // IMU
    Q_PROPERTY(int imuYaw    READ imuYaw    NOTIFY updated)
    Q_PROPERTY(int imuPitch  READ imuPitch  NOTIFY updated)
    Q_PROPERTY(int imuRoll   READ imuRoll   NOTIFY updated)
    Q_PROPERTY(int imuAccelX READ imuAccelX NOTIFY updated)
    Q_PROPERTY(int imuAccelY READ imuAccelY NOTIFY updated)
    Q_PROPERTY(int imuAccelZ READ imuAccelZ NOTIFY updated)
    Q_PROPERTY(int imuGyroX  READ imuGyroX  NOTIFY updated)
    Q_PROPERTY(int imuGyroY  READ imuGyroY  NOTIFY updated)
    Q_PROPERTY(int imuGyroZ  READ imuGyroZ  NOTIFY updated)
    Q_PROPERTY(int imuTemp   READ imuTemp   NOTIFY updated)

    // 左电机
    Q_PROPERTY(int leftSpeed   READ leftSpeed   NOTIFY updated)
    Q_PROPERTY(int leftCurrent READ leftCurrent NOTIFY updated)
    Q_PROPERTY(int leftEncoder READ leftEncoder NOTIFY updated)
    Q_PROPERTY(int leftFault   READ leftFault   NOTIFY updated)

    // 右电机
    Q_PROPERTY(int rightSpeed   READ rightSpeed   NOTIFY updated)
    Q_PROPERTY(int rightCurrent READ rightCurrent NOTIFY updated)
    Q_PROPERTY(int rightEncoder READ rightEncoder NOTIFY updated)
    Q_PROPERTY(int rightFault   READ rightFault   NOTIFY updated)

    // 切割电机
    Q_PROPERTY(int cutSpeed   READ cutSpeed   NOTIFY updated)
    Q_PROPERTY(int cutCurrent READ cutCurrent NOTIFY updated)
    Q_PROPERTY(int cutEncoder READ cutEncoder NOTIFY updated)
    Q_PROPERTY(int cutFault   READ cutFault   NOTIFY updated)

    // 电池
    Q_PROPERTY(int batterySoc  READ batterySoc  NOTIFY updated)
    Q_PROPERTY(int batteryTemp READ batteryTemp NOTIFY updated)

    // 系统/传感器标志
    Q_PROPERTY(int systemFlags      READ systemFlags      NOTIFY updated)
    Q_PROPERTY(int sensorFlags      READ sensorFlags      NOTIFY updated)
    Q_PROPERTY(int moduleFaultFlags READ moduleFaultFlags NOTIFY updated)
    Q_PROPERTY(int keyEventFlags    READ keyEventFlags    NOTIFY updated)

    // 帧统计
    Q_PROPERTY(int frameCount READ frameCount NOTIFY updated)

public:
    explicit McuReportModel(QObject *parent = nullptr);

    // 收到真实串口帧时调用
    void update(const gnu_soc_proto_mcu_report_t &r);

    // 启动模拟数据（无硬件时用于界面预览）
    void startMock();

    int vReal()    const { return m_data.v_real; }
    int wReal()    const { return m_data.w_real; }

    int imuYaw()    const { return m_data.imu.yaw; }
    int imuPitch()  const { return m_data.imu.pitch; }
    int imuRoll()   const { return m_data.imu.roll; }
    int imuAccelX() const { return m_data.imu.accel_x; }
    int imuAccelY() const { return m_data.imu.accel_y; }
    int imuAccelZ() const { return m_data.imu.accel_z; }
    int imuGyroX()  const { return m_data.imu.gyro_x; }
    int imuGyroY()  const { return m_data.imu.gyro_y; }
    int imuGyroZ()  const { return m_data.imu.gyro_z; }
    int imuTemp()   const { return m_data.imu.temperature; }

    int leftSpeed()   const { return m_data.left.speed; }
    int leftCurrent() const { return m_data.left.current; }
    int leftEncoder() const { return m_data.left.encoder; }
    int leftFault()   const { return m_data.left.fault_code; }

    int rightSpeed()   const { return m_data.right.speed; }
    int rightCurrent() const { return m_data.right.current; }
    int rightEncoder() const { return m_data.right.encoder; }
    int rightFault()   const { return m_data.right.fault_code; }

    int cutSpeed()   const { return m_data.cut.speed; }
    int cutCurrent() const { return m_data.cut.current; }
    int cutEncoder() const { return m_data.cut.encoder; }
    int cutFault()   const { return m_data.cut.fault_code; }

    int batterySoc()  const { return m_data.battery.soc; }
    int batteryTemp() const { return m_data.battery.temperature; }

    int systemFlags()      const { return m_data.system.system_flags; }
    int sensorFlags()      const { return m_data.system.sensor_flags; }
    int moduleFaultFlags() const { return m_data.system.module_fault_flags; }
    int keyEventFlags()    const { return m_data.system.key_event_flags; }

    int frameCount() const { return m_frameCount; }

signals:
    void updated();

private:
    gnu_soc_proto_mcu_report_t m_data{};
    int m_frameCount = 0;
    int m_mockTick   = 0;
    QTimer m_mockTimer;
};
