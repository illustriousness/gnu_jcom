#include "McuReportModel.h"
#include <cmath>

McuReportModel::McuReportModel(QObject *parent) : QObject(parent)
{
    connect(&m_mockTimer, &QTimer::timeout, this, [this]() {
        const double t = m_mockTick * 0.1;
        m_mockTick++;

        // 运动：正弦波模拟加速/减速
        m_data.v_real = static_cast<int16_t>(300.0 * std::sin(t * 0.5));
        m_data.w_real = static_cast<int16_t>(200.0 * std::cos(t * 0.3));

        // IMU：缓慢旋转 + 振动
        m_data.imu.yaw         = static_cast<int16_t>(std::fmod(t * 5.0, 360.0));
        m_data.imu.pitch       = static_cast<int16_t>(15.0 * std::sin(t * 0.8));
        m_data.imu.roll        = static_cast<int16_t>(10.0 * std::cos(t * 0.6));
        m_data.imu.accel_x     = static_cast<int16_t>(50.0  * std::sin(t * 1.0));
        m_data.imu.accel_y     = static_cast<int16_t>(50.0  * std::cos(t * 1.2));
        m_data.imu.accel_z     = static_cast<int16_t>(1000.0 + 20.0 * std::sin(t * 2.0));
        m_data.imu.gyro_x      = static_cast<int16_t>(10.0  * std::sin(t * 1.5));
        m_data.imu.gyro_y      = static_cast<int16_t>(10.0  * std::cos(t * 1.7));
        m_data.imu.gyro_z      = static_cast<int16_t>(5.0   * std::sin(t * 0.4));
        m_data.imu.temperature = static_cast<int16_t>(250.0 + 5.0 * std::sin(t * 0.1));

        // 左/右电机：同向驱动
        const int16_t driveSpeed = static_cast<int16_t>(250.0 * std::sin(t * 0.5));
        m_data.left.speed    = driveSpeed;
        m_data.left.current  = static_cast<uint16_t>(500.0 + 100.0 * std::abs(std::sin(t * 0.5)));
        m_data.left.encoder += driveSpeed / 10;
        m_data.left.fault_code = 0;

        m_data.right.speed    = driveSpeed;
        m_data.right.current  = static_cast<uint16_t>(500.0 + 100.0 * std::abs(std::cos(t * 0.5)));
        m_data.right.encoder += driveSpeed / 10;
        m_data.right.fault_code = 0;

        // 切割电机：高速常转
        m_data.cut.speed    = static_cast<int16_t>(1200.0 + 100.0 * std::sin(t * 2.0));
        m_data.cut.current  = static_cast<uint16_t>(800.0 + 200.0 * std::abs(std::sin(t)));
        m_data.cut.encoder += m_data.cut.speed / 5;
        m_data.cut.fault_code = 0;

        // 电池
        m_data.battery.soc         = 75;
        m_data.battery.temperature = static_cast<int16_t>(28.0 + 1.0 * std::sin(t * 0.05));

        // 系统标志：READY | RUNNING
        m_data.system.system_flags      = 0x0003;
        m_data.system.sensor_flags      = 0x0000;
        m_data.system.module_fault_flags = 0x0000;
        m_data.system.key_event_flags   = 0x0000;

        m_frameCount++;
        emit updated();
    });
}

void McuReportModel::update(const gnu_soc_proto_mcu_report_t &r)
{
    m_data = r;
    m_frameCount++;
    emit updated();
}

void McuReportModel::startMock()
{
    m_mockTimer.start(100); // 10 Hz
}
