#include "payloadcodec.h"

#include <QtTest>

class PayloadCodecTest : public QObject
{
    Q_OBJECT

private slots:
    void encodesAsciiWithLineEnding();
    void parsesHexWithSeparators();
    void rejectsOddHexLength();
};

void PayloadCodecTest::encodesAsciiWithLineEnding()
{
    QString errorMessage;
    const QByteArray result = PayloadCodec::encodePayload(
        QStringLiteral("AT"),
        PayloadCodec::PayloadMode::Ascii,
        PayloadCodec::LineEnding::CrLf,
        &errorMessage);

    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(result, QByteArray("AT\r\n"));
}

void PayloadCodecTest::parsesHexWithSeparators()
{
    QString errorMessage;
    const QByteArray result = PayloadCodec::encodePayload(
        QStringLiteral("41 54 0D 0A"),
        PayloadCodec::PayloadMode::Hex,
        PayloadCodec::LineEnding::None,
        &errorMessage);

    QVERIFY(errorMessage.isEmpty());
    QCOMPARE(result, QByteArray("AT\r\n"));
}

void PayloadCodecTest::rejectsOddHexLength()
{
    QString errorMessage;
    const QByteArray result = PayloadCodec::encodePayload(
        QStringLiteral("0A B"),
        PayloadCodec::PayloadMode::Hex,
        PayloadCodec::LineEnding::None,
        &errorMessage);

    QVERIFY(result.isEmpty());
    QVERIFY(!errorMessage.isEmpty());
}

QTEST_MAIN(PayloadCodecTest)

#include "payloadcodec_test.moc"
