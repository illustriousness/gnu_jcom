#include "packetschema.h"

#include <QtTest>

class PacketSchemaTest : public QObject
{
    Q_OBJECT

private slots:
    void loadsSchema();
    void buildsFrame();
    void parsesFrame();
    void supportsU64WithFooter();
    void supportsGenericFieldTypes();
    void supportsChecksumStartOffset();
    void computesCommonCrcs();
};

void PacketSchemaTest::loadsSchema()
{
    const QByteArray json = R"({
        "name": "demo",
        "header": "AA55",
        "checksum": "sum8",
        "fields": [
            {"name":"seq","type":"u8","precision":0,"defaultValue":"0"},
            {"name":"value","type":"i16","scale":0.01,"precision":2,"unit":"deg","chart":true,"defaultValue":"0"}
        ]
    })";

    PacketSchema schema;
    QString errorMessage;
    QVERIFY(loadPacketSchemaFromJson(json, &schema, &errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(schema.name, QStringLiteral("demo"));
    QCOMPARE(schema.header, QByteArray::fromHex("AA55"));
    QCOMPARE(schema.frameSize(), 6);
}

void PacketSchemaTest::buildsFrame()
{
    const QByteArray json = R"({
        "name": "demo",
        "header": "AA55",
        "checksum": "sum8",
        "fields": [
            {"name":"seq","type":"u8","precision":0,"defaultValue":"0"},
            {"name":"value","type":"i16","scale":0.01,"precision":2,"defaultValue":"0"}
        ]
    })";

    PacketSchema schema;
    QString errorMessage;
    QVERIFY(loadPacketSchemaFromJson(json, &schema, &errorMessage));

    const QByteArray frame = buildPacketFrame(schema, {QStringLiteral("1"), QStringLiteral("12.34")}, &errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(frame.toHex().toUpper(), QByteArray("AA5501D204D6"));
}

void PacketSchemaTest::parsesFrame()
{
    const QByteArray json = R"({
        "name": "demo",
        "header": "AA55",
        "checksum": "sum8",
        "fields": [
            {"name":"seq","type":"u8","precision":0,"defaultValue":"0"},
            {"name":"value","type":"i16","scale":0.01,"precision":2,"unit":"deg","defaultValue":"0"}
        ]
    })";

    PacketSchema schema;
    QString errorMessage;
    QVERIFY(loadPacketSchemaFromJson(json, &schema, &errorMessage));

    QVector<double> numericValues;
    QStringList displayValues;
    QVERIFY(parsePacketFrame(schema,
                             QByteArray::fromHex("AA5501D204D6"),
                             &numericValues,
                             &displayValues,
                             &errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(numericValues.size(), 2);
    QCOMPARE(numericValues.at(0), 1.0);
    QCOMPARE(numericValues.at(1), 12.34);
    QCOMPARE(displayValues.at(1), QStringLiteral("12.34 deg"));
}

void PacketSchemaTest::supportsU64WithFooter()
{
    const QByteArray json = R"({
        "name": "u64_demo",
        "header": "AA55",
        "footer": "0D0A",
        "checksum": "none",
        "fields": [
            {"name":"frame_id","type":"u64","precision":0,"defaultValue":"0"}
        ]
    })";

    PacketSchema schema;
    QString errorMessage;
    QVERIFY(loadPacketSchemaFromJson(json, &schema, &errorMessage));

    const QString maxValue = QStringLiteral("18446744073709551615");
    const QByteArray frame = buildPacketFrame(schema, {maxValue}, &errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(frame.toHex().toUpper(), QByteArray("AA55FFFFFFFFFFFFFFFF0D0A"));

    QVector<double> numericValues;
    QStringList displayValues;
    QVERIFY(parsePacketFrame(schema, frame, &numericValues, &displayValues, &errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(displayValues.size(), 1);
    QCOMPARE(displayValues.at(0), maxValue);
}

void PacketSchemaTest::supportsGenericFieldTypes()
{
    const QByteArray json = R"({
        "name": "generic_types",
        "header": "A55A",
        "checksum": "none",
        "fields": [
            {"name":"a","type":"uint","byteWidth":1,"precision":0,"defaultValue":"1"},
            {"name":"b","type":"int","byteWidth":2,"precision":0,"defaultValue":"-2"},
            {"name":"c","type":"float","byteWidth":4,"precision":2,"defaultValue":"3.25"}
        ]
    })";

    PacketSchema schema;
    QString errorMessage;
    QVERIFY(loadPacketSchemaFromJson(json, &schema, &errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(schema.fields.at(0).byteWidth, 1);
    QCOMPARE(schema.fields.at(1).byteWidth, 2);
    QCOMPARE(schema.fields.at(2).byteWidth, 4);

    const QByteArray frame = buildPacketFrame(schema,
                                              {QStringLiteral("1"),
                                               QStringLiteral("-2"),
                                               QStringLiteral("3.25")},
                                              &errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(frame.toHex().toUpper(), QByteArray("A55A01FEFF00005040"));

    QVector<double> numericValues;
    QStringList displayValues;
    QVERIFY(parsePacketFrame(schema, frame, &numericValues, &displayValues, &errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(displayValues.at(0), QStringLiteral("1"));
    QCOMPARE(displayValues.at(1), QStringLiteral("-2"));
    QCOMPARE(displayValues.at(2), QStringLiteral("3.25"));
}

void PacketSchemaTest::supportsChecksumStartOffset()
{
    const QByteArray json = R"({
        "name": "len_crc_demo",
        "header": "AA55",
        "checksum": "crc16_ccitt_false_le",
        "checksumStart": 2,
        "fields": [
            {"name":"len","type":"u8","precision":0,"defaultValue":"2"},
            {"name":"value","type":"u16","endian":"little","precision":0,"defaultValue":"0"}
        ]
    })";

    PacketSchema schema;
    QString errorMessage;
    QVERIFY(loadPacketSchemaFromJson(json, &schema, &errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(schema.checksumStart, 2);

    const QByteArray frame = buildPacketFrame(schema,
                                              {QStringLiteral("2"), QStringLiteral("4660")},
                                              &errorMessage);
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(frame.toHex().toUpper(), QByteArray("AA55023412DE59"));

    QVector<double> numericValues;
    QStringList displayValues;
    QVERIFY(parsePacketFrame(schema, frame, &numericValues, &displayValues, &errorMessage));
    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(displayValues.at(0), QStringLiteral("2"));
    QCOMPARE(displayValues.at(1), QStringLiteral("4660"));
}

void PacketSchemaTest::computesCommonCrcs()
{
    QString errorMessage;
    QCOMPARE(buildPacketChecksumBytes(QStringLiteral("crc8"), QByteArray("123456789"), &errorMessage)
                 .toHex()
                 .toUpper(),
             QByteArray("F4"));
    QVERIFY(errorMessage.isEmpty());

    QCOMPARE(buildPacketChecksumBytes(QStringLiteral("crc16_modbus"), QByteArray("123456789"), &errorMessage)
                 .toHex()
                 .toUpper(),
             QByteArray("374B"));
    QVERIFY(errorMessage.isEmpty());

    QCOMPARE(buildPacketChecksumBytes(QStringLiteral("crc16_ccitt_false"), QByteArray("123456789"), &errorMessage)
                 .toHex()
                 .toUpper(),
             QByteArray("29B1"));
    QVERIFY(errorMessage.isEmpty());

    QCOMPARE(buildPacketChecksumBytes(QStringLiteral("crc16_ccitt_false_le"), QByteArray("123456789"), &errorMessage)
                 .toHex()
                 .toUpper(),
             QByteArray("B129"));
    QVERIFY(errorMessage.isEmpty());

    QCOMPARE(buildPacketChecksumBytes(QStringLiteral("crc32"), QByteArray("123456789"), &errorMessage)
                 .toHex()
                 .toUpper(),
             QByteArray("2639F4CB"));
    QVERIFY(errorMessage.isEmpty());
}

QTEST_MAIN(PacketSchemaTest)

#include "packetschema_test.moc"
