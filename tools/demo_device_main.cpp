#include "packetschema.h"

#include <QCoreApplication>
#include <QElapsedTimer>
#include <QFileInfo>
#include <QSerialPort>
#include <QTimer>

#include <cmath>

namespace {

QString defaultSchemaPath()
{
    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates{
        QFileInfo(appDir + QStringLiteral("/../../../share/gnu_jcom/examples/soc_proto_mcu_report_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../../../share/gnu_jcom/examples/linear_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../../../share/gnu_jcom/examples/sine_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../share/gnu_jcom/examples/soc_proto_mcu_report_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../share/gnu_jcom/examples/linear_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../share/gnu_jcom/examples/sine_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../examples/soc_proto_mcu_report_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../examples/linear_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/../examples/sine_demo_schema.json")).absoluteFilePath(),
        QFileInfo(appDir + QStringLiteral("/examples/soc_proto_mcu_report_schema.json")).absoluteFilePath(),
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

} // namespace

class DemoDevice final : public QObject
{
    Q_OBJECT

public:
    DemoDevice(const QString &portName,
               int baudRate,
               const QString &schemaPath,
               int intervalMs,
               QObject *parent = nullptr)
        : QObject(parent)
    {
        QString errorMessage;
        if (!loadPacketSchemaFromFile(schemaPath, &m_schema, &errorMessage)) {
            qFatal("Failed to load schema %s: %s",
                   qPrintable(schemaPath),
                   qPrintable(errorMessage));
        }

        m_serial.setPortName(portName);
        m_serial.setBaudRate(baudRate);

        if (!m_serial.open(QIODevice::ReadWrite)) {
            qFatal("Failed to open %s: %s",
                   qPrintable(portName),
                   qPrintable(m_serial.errorString()));
        }

        connect(&m_timer, &QTimer::timeout, this, &DemoDevice::sendFrame);
        m_timer.start(intervalMs);
        m_clock.start();

        qInfo("Demo device started: port=%s baud=%d schema=%s interval=%dms",
              qPrintable(portName),
              baudRate,
              qPrintable(schemaPath),
              intervalMs);
    }

private:
    void sendFrame()
    {
        const double t = static_cast<double>(m_clock.elapsed()) / 1000.0;
        const double sine = std::sin(t * 1.7) * 100.0;
        const double cosine = std::cos(t * 1.7) * 100.0;
        const double slow = std::sin(t * 0.7) * 60.0;
        const int linear = m_linearValue;

        QStringList values;
        values.reserve(m_schema.fields.size());
        for (const PacketFieldDef &field : m_schema.fields) {
            const QString name = field.name.trimmed().toLower();
            if (name == QStringLiteral("seq")) {
                values.append(QString::number(m_sequence));
            } else if (name == QStringLiteral("x")
                       || name == QStringLiteral("value")
                       || name == QStringLiteral("base")) {
                values.append(QString::number(linear));
            } else if (name == QStringLiteral("x2")
                       || name == QStringLiteral("2x")) {
                values.append(QString::number(linear * 2));
            } else if (name == QStringLiteral("x3")
                       || name == QStringLiteral("3x")) {
                values.append(QString::number(linear * 3));
            } else if (name == QStringLiteral("sine")) {
                values.append(QString::number(sine, 'f', 2));
            } else if (name == QStringLiteral("cosine")) {
                values.append(QString::number(cosine, 'f', 2));
            } else if (name == QStringLiteral("slow")) {
                values.append(QString::number(slow, 'f', 2));
            } else {
                values.append(QString::number(linear));
            }
        }

        QString errorMessage;
        const QByteArray frame = buildPacketFrame(m_schema, values, &errorMessage);
        if (frame.isEmpty()) {
            qWarning("Failed to build demo frame: %s", qPrintable(errorMessage));
            return;
        }

        m_serial.write(frame);
        m_sequence = static_cast<quint8>(m_sequence + 1);
        m_linearValue = (m_linearValue + 1) % 1000;
    }

    PacketSchema m_schema;
    QSerialPort m_serial;
    QTimer m_timer;
    QElapsedTimer m_clock;
    quint8 m_sequence = 0;
    int m_linearValue = 1;
};

int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("gnu_jcom_demo_device"));

    const QStringList arguments = app.arguments();
    if (arguments.size() < 2) {
        qInfo("Usage: %s <serial-port> [baud-rate] [schema-path] [interval-ms]",
              qPrintable(arguments.first()));
        qInfo("Example: %s /dev/tnt1 115200 %s 1000",
              qPrintable(arguments.first()),
              qPrintable(defaultSchemaPath()));
        return 1;
    }

    const QString portName = arguments.at(1);
    const int baudRate = arguments.size() >= 3 ? arguments.at(2).toInt() : 115200;
    const QString schemaPath = arguments.size() >= 4 ? arguments.at(3) : defaultSchemaPath();
    const int intervalMs = arguments.size() >= 5 ? arguments.at(4).toInt() : 20;

    if (intervalMs <= 0) {
        qCritical("interval-ms must be greater than zero.");
        return 1;
    }

    DemoDevice device(portName, baudRate, schemaPath, intervalMs);
    return app.exec();
}

#include "demo_device_main.moc"
