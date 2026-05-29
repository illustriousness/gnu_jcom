#pragma once

#include <QByteArray>
#include <QString>

namespace PayloadCodec {

enum class PayloadMode {
    Ascii = 0,
    Hex = 1,
};

enum class LineEnding {
    None = 0,
    Lf = 1,
    CrLf = 2,
    Cr = 3,
};

QByteArray encodePayload(const QString &payload,
                         PayloadMode mode,
                         LineEnding lineEnding,
                         QString *errorMessage = nullptr);

QString asciiPreview(const QByteArray &bytes);
QString hexPreview(const QByteArray &bytes);

} // namespace PayloadCodec

