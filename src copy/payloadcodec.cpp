#include "payloadcodec.h"

#include <QChar>
#include <QRegularExpression>

namespace {

QByteArray appendLineEnding(QByteArray bytes, PayloadCodec::LineEnding lineEnding)
{
    switch (lineEnding) {
    case PayloadCodec::LineEnding::Lf:
        bytes.append('\n');
        break;
    case PayloadCodec::LineEnding::CrLf:
        bytes.append("\r\n");
        break;
    case PayloadCodec::LineEnding::Cr:
        bytes.append('\r');
        break;
    case PayloadCodec::LineEnding::None:
    default:
        break;
    }

    return bytes;
}

QString sanitizeHexInput(QString payload)
{
    payload.remove(QRegularExpression(QStringLiteral("0[xX]")));
    payload.remove(QRegularExpression(QStringLiteral(R"([\s,:;_\-])")));
    return payload.toUpper();
}

} // namespace

namespace PayloadCodec {

QByteArray encodePayload(const QString &payload,
                         PayloadMode mode,
                         LineEnding lineEnding,
                         QString *errorMessage)
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    QByteArray bytes;

    if (mode == PayloadMode::Ascii) {
        bytes = payload.toUtf8();
    } else {
        const QString sanitized = sanitizeHexInput(payload);
        if (sanitized.isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("HEX payload is empty.");
            }
            return {};
        }

        if ((sanitized.size() % 2) != 0) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("HEX payload must contain an even number of digits.");
            }
            return {};
        }

        bytes.reserve(sanitized.size() / 2);
        for (qsizetype index = 0; index < sanitized.size(); index += 2) {
            const QString pair = sanitized.mid(index, 2);
            bool ok = false;
            const int value = pair.toInt(&ok, 16);
            if (!ok) {
                if (errorMessage != nullptr) {
                    *errorMessage = QStringLiteral("Invalid HEX byte: %1").arg(pair);
                }
                return {};
            }

            bytes.append(static_cast<char>(value));
        }
    }

    return appendLineEnding(bytes, lineEnding);
}

QString asciiPreview(const QByteArray &bytes)
{
    QString output;
    output.reserve(bytes.size());

    for (const char byte : bytes) {
        switch (byte) {
        case '\r':
            output.append(QStringLiteral("\\r"));
            break;
        case '\n':
            output.append(QStringLiteral("\\n"));
            break;
        case '\t':
            output.append(QStringLiteral("\\t"));
            break;
        default: {
            const unsigned char code = static_cast<unsigned char>(byte);
            if (code >= 32 && code <= 126) {
                output.append(QChar::fromLatin1(byte));
            } else {
                output.append(QChar('.'));
            }
            break;
        }
        }
    }

    return output;
}

QString hexPreview(const QByteArray &bytes)
{
    QString output;
    output.reserve((bytes.size() * 3) - (bytes.isEmpty() ? 0 : 1));

    for (qsizetype index = 0; index < bytes.size(); ++index) {
        if (index > 0) {
            output.append(QChar(' '));
        }

        const unsigned char value = static_cast<unsigned char>(bytes.at(index));
        output.append(QStringLiteral("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper());
    }

    return output;
}

} // namespace PayloadCodec

