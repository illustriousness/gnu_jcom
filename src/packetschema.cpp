#include "packetschema.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

#include <cmath>
#include <cstring>
#include <limits>

namespace {

struct PacketChecksumDef {
    const char *name;
    int bitWidth;
    quint64 polynomial;
    quint64 init;
    bool reflectInput;
    bool reflectOutput;
    quint64 xorOutput;
    bool littleEndian;
    bool sum8;
};

constexpr PacketChecksumDef kPacketChecksumDefs[] = {
    {"none", 0, 0, 0, false, false, 0, true, false},
    {"sum8", 8, 0, 0, false, false, 0, true, true},
    {"crc8", 8, 0x07, 0x00, false, false, 0x00, true, false},
    {"crc8_itu", 8, 0x07, 0x00, false, false, 0x55, true, false},
    {"crc8_rohc", 8, 0x07, 0xFF, true, true, 0x00, true, false},
    {"crc8_maxim", 8, 0x31, 0x00, true, true, 0x00, true, false},
    {"crc16_ibm", 16, 0x8005, 0x0000, true, true, 0x0000, true, false},
    {"crc16_maxim", 16, 0x8005, 0x0000, true, true, 0xFFFF, true, false},
    {"crc16_usb", 16, 0x8005, 0xFFFF, true, true, 0xFFFF, true, false},
    {"crc16_modbus", 16, 0x8005, 0xFFFF, true, true, 0x0000, true, false},
    {"crc16_ccitt", 16, 0x1021, 0x0000, true, true, 0x0000, true, false},
    {"crc16_ccitt_false", 16, 0x1021, 0xFFFF, false, false, 0x0000, false, false},
    {"crc16_x25", 16, 0x1021, 0xFFFF, true, true, 0xFFFF, true, false},
    {"crc16_xmodem", 16, 0x1021, 0x0000, false, false, 0x0000, false, false},
    {"crc16_dnp", 16, 0x3D65, 0x0000, true, true, 0xFFFF, true, false},
    {"crc32", 32, 0x04C11DB7, 0xFFFFFFFF, true, true, 0xFFFFFFFF, true, false},
    {"crc32_mpeg2", 32, 0x04C11DB7, 0xFFFFFFFF, false, false, 0x00000000, false, false},
};

QString sanitizeHex(QString text)
{
    text.remove(QRegularExpression(QStringLiteral("0[xX]")));
    text.remove(QRegularExpression(QStringLiteral(R"([\s,:;_\-])")));
    return text.toUpper();
}

QByteArray parseHexBytes(const QString &text, const QString &label, QString *errorMessage)
{
    const QString sanitized = sanitizeHex(text);
    if (sanitized.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("%1 is empty.").arg(label);
        }
        return {};
    }

    if ((sanitized.size() % 2) != 0) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("%1 must contain an even number of digits.").arg(label);
        }
        return {};
    }

    const QByteArray bytes = QByteArray::fromHex(sanitized.toLatin1());
    if (bytes.isEmpty() && !sanitized.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("%1 is not valid HEX.").arg(label);
        }
    }
    return bytes;
}

QString formatDisplayValue(double value, const PacketFieldDef &field)
{
    const int precision = qMax(0, field.precision);
    QString text = QString::number(value, 'f', precision);
    if (!field.unit.isEmpty()) {
        text += QStringLiteral(" %1").arg(field.unit);
    }
    return text;
}

QString formatIntegerDisplay(quint64 value, const PacketFieldDef &field)
{
    if (!qFuzzyCompare(field.scale, 1.0)) {
        return formatDisplayValue(static_cast<double>(value) * field.scale, field);
    }

    QString text = QString::number(value);
    if (!field.unit.isEmpty()) {
        text += QStringLiteral(" %1").arg(field.unit);
    }
    return text;
}

QString formatSignedDisplay(qint64 value, const PacketFieldDef &field)
{
    if (!qFuzzyCompare(field.scale, 1.0)) {
        return formatDisplayValue(static_cast<double>(value) * field.scale, field);
    }

    QString text = QString::number(value);
    if (!field.unit.isEmpty()) {
        text += QStringLiteral(" %1").arg(field.unit);
    }
    return text;
}

bool parseUnsignedIntegerInput(const QString &input, quint64 *value)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    bool ok = false;
    const int base = trimmed.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive) ? 16 : 10;
    const QString digits = base == 16 ? trimmed.mid(2) : trimmed;
    const quint64 parsedValue = digits.toULongLong(&ok, base);
    if (!ok) {
        return false;
    }

    if (value != nullptr) {
        *value = parsedValue;
    }
    return true;
}

bool parseSignedIntegerInput(const QString &input, qint64 *value)
{
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        return false;
    }

    bool ok = false;
    const qint64 parsedValue = trimmed.toLongLong(&ok, 10);
    if (!ok) {
        return false;
    }

    if (value != nullptr) {
        *value = parsedValue;
    }
    return true;
}

quint64 unsignedMaximumForWidth(int byteWidth)
{
    if (byteWidth >= 8) {
        return std::numeric_limits<quint64>::max();
    }

    return (quint64{1} << (byteWidth * 8)) - 1;
}

qint64 signedMinimumForWidth(int byteWidth)
{
    if (byteWidth >= 8) {
        return std::numeric_limits<qint64>::min();
    }

    return -(qint64{1} << ((byteWidth * 8) - 1));
}

qint64 signedMaximumForWidth(int byteWidth)
{
    if (byteWidth >= 8) {
        return std::numeric_limits<qint64>::max();
    }

    return (qint64{1} << ((byteWidth * 8) - 1)) - 1;
}

quint64 maskForWidth(int byteWidth)
{
    return unsignedMaximumForWidth(byteWidth);
}

bool isValidByteWidth(PacketValueType type, int byteWidth)
{
    if (byteWidth != 1 && byteWidth != 2 && byteWidth != 4 && byteWidth != 8) {
        return false;
    }

    if (type == PacketValueType::Float) {
        return byteWidth == 4 || byteWidth == 8;
    }

    return type == PacketValueType::UInt || type == PacketValueType::Int;
}

bool resolvePacketFieldType(const QString &rawTypeName,
                            int requestedByteWidth,
                            PacketValueType *type,
                            int *byteWidth)
{
    const QString normalized = rawTypeName.trimmed().toLower();

    auto assign = [type, byteWidth](PacketValueType resolvedType, int resolvedByteWidth) {
        if (type != nullptr) {
            *type = resolvedType;
        }
        if (byteWidth != nullptr) {
            *byteWidth = resolvedByteWidth;
        }
        return true;
    };

    if (normalized == QStringLiteral("uint")) {
        return assign(PacketValueType::UInt, requestedByteWidth > 0 ? requestedByteWidth : 2);
    }
    if (normalized == QStringLiteral("int")) {
        return assign(PacketValueType::Int, requestedByteWidth > 0 ? requestedByteWidth : 2);
    }
    if (normalized == QStringLiteral("float")) {
        int resolvedWidth = requestedByteWidth > 0 ? requestedByteWidth : 4;
        if (resolvedWidth != 4 && resolvedWidth != 8) {
            resolvedWidth = 4;
        }
        return assign(PacketValueType::Float, resolvedWidth);
    }
    if (normalized == QStringLiteral("u8")) {
        return assign(PacketValueType::UInt, 1);
    }
    if (normalized == QStringLiteral("u16")) {
        return assign(PacketValueType::UInt, 2);
    }
    if (normalized == QStringLiteral("u32")) {
        return assign(PacketValueType::UInt, 4);
    }
    if (normalized == QStringLiteral("u64")) {
        return assign(PacketValueType::UInt, 8);
    }
    if (normalized == QStringLiteral("i8")) {
        return assign(PacketValueType::Int, 1);
    }
    if (normalized == QStringLiteral("i16")) {
        return assign(PacketValueType::Int, 2);
    }
    if (normalized == QStringLiteral("i32")) {
        return assign(PacketValueType::Int, 4);
    }
    if (normalized == QStringLiteral("i64")) {
        return assign(PacketValueType::Int, 8);
    }
    if (normalized == QStringLiteral("f32")) {
        return assign(PacketValueType::Float, 4);
    }
    if (normalized == QStringLiteral("f64")) {
        return assign(PacketValueType::Float, 8);
    }

    return false;
}

QString typeLabel(const PacketFieldDef &field)
{
    if (field.type == PacketValueType::UInt) {
        return QStringLiteral("unsigned %1-byte integer value").arg(field.byteWidth);
    }
    if (field.type == PacketValueType::Int) {
        return QStringLiteral("signed %1-byte integer value").arg(field.byteWidth);
    }
    if (field.type == PacketValueType::Float) {
        return QStringLiteral("%1-byte float value").arg(field.byteWidth);
    }

    return QStringLiteral("field value");
}

bool parseUnsignedFieldInput(const PacketFieldDef &field,
                             const QString &input,
                             quint64 maximum,
                             quint64 *value,
                             QString *errorMessage)
{
    const QString trimmed = input.trimmed();
    const QString label = typeLabel(field);
    if (trimmed.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("invalid %1.").arg(label);
        }
        return false;
    }

    quint64 parsedInteger = 0;
    if (qFuzzyCompare(field.scale, 1.0) && parseUnsignedIntegerInput(trimmed, &parsedInteger)) {
        if (parsedInteger > maximum) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("%1 is out of range.").arg(label);
            }
            return false;
        }

        if (value != nullptr) {
            *value = parsedInteger;
        }
        return true;
    }

    bool ok = false;
    const double displayValue = trimmed.toDouble(&ok);
    if (!ok) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("invalid %1.").arg(label);
        }
        return false;
    }

    const double rawValue = displayValue / field.scale;
    if (!std::isfinite(rawValue) || rawValue < 0.0 || rawValue > static_cast<double>(maximum)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("%1 is out of range.").arg(label);
        }
        return false;
    }

    if (value != nullptr) {
        *value = static_cast<quint64>(std::llround(rawValue));
    }
    return true;
}

bool parseSignedFieldInput(const PacketFieldDef &field,
                           const QString &input,
                           qint64 minimum,
                           qint64 maximum,
                           qint64 *value,
                           QString *errorMessage)
{
    const QString trimmed = input.trimmed();
    const QString label = typeLabel(field);
    if (trimmed.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("invalid %1.").arg(label);
        }
        return false;
    }

    qint64 parsedInteger = 0;
    if (qFuzzyCompare(field.scale, 1.0) && parseSignedIntegerInput(trimmed, &parsedInteger)) {
        if (parsedInteger < minimum || parsedInteger > maximum) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("%1 is out of range.").arg(label);
            }
            return false;
        }

        if (value != nullptr) {
            *value = parsedInteger;
        }
        return true;
    }

    bool ok = false;
    const double displayValue = trimmed.toDouble(&ok);
    if (!ok) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("invalid %1.").arg(label);
        }
        return false;
    }

    const double rawValue = displayValue / field.scale;
    if (!std::isfinite(rawValue)
        || rawValue < static_cast<double>(minimum)
        || rawValue > static_cast<double>(maximum)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("%1 is out of range.").arg(label);
        }
        return false;
    }

    if (value != nullptr) {
        *value = static_cast<qint64>(std::llround(rawValue));
    }
    return true;
}

QByteArray encodeUnsignedInteger(quint64 value, int byteWidth, bool littleEndian)
{
    QByteArray bytes(byteWidth, Qt::Uninitialized);
    for (int byteIndex = 0; byteIndex < byteWidth; ++byteIndex) {
        const int shift = littleEndian ? (byteIndex * 8) : ((byteWidth - byteIndex - 1) * 8);
        bytes[byteIndex] = static_cast<char>((value >> shift) & 0xFF);
    }
    return bytes;
}

quint64 decodeUnsignedInteger(const QByteArray &bytes, bool littleEndian)
{
    quint64 value = 0;
    if (littleEndian) {
        for (int byteIndex = 0; byteIndex < bytes.size(); ++byteIndex) {
            value |= (static_cast<quint64>(static_cast<quint8>(bytes.at(byteIndex))) << (byteIndex * 8));
        }
        return value;
    }

    for (int byteIndex = 0; byteIndex < bytes.size(); ++byteIndex) {
        value = (value << 8) | static_cast<quint64>(static_cast<quint8>(bytes.at(byteIndex)));
    }
    return value;
}

quint64 reflectBits(quint64 value, int bitCount)
{
    quint64 reflected = 0;
    for (int bitIndex = 0; bitIndex < bitCount; ++bitIndex) {
        if (value & (quint64{1} << bitIndex)) {
            reflected |= quint64{1} << ((bitCount - 1) - bitIndex);
        }
    }
    return reflected;
}

quint64 crcMask(int bitWidth)
{
    if (bitWidth >= 64) {
        return std::numeric_limits<quint64>::max();
    }
    return (quint64{1} << bitWidth) - 1;
}

const PacketChecksumDef *packetChecksumDefinition(const QString &checksumName)
{
    const QString normalized = checksumName.trimmed().toLower();
    for (const PacketChecksumDef &definition : kPacketChecksumDefs) {
        if (normalized == QLatin1String(definition.name)) {
            return &definition;
        }
    }
    return nullptr;
}

quint64 computeCrc(const PacketChecksumDef &definition, const QByteArray &bytes)
{
    const quint64 mask = crcMask(definition.bitWidth);
    const quint64 topBit = quint64{1} << (definition.bitWidth - 1);
    quint64 crc = definition.init & mask;

    for (const char byte : bytes) {
        quint64 data = static_cast<quint8>(byte);
        if (definition.reflectInput) {
            data = reflectBits(data, 8);
        }

        crc ^= (data << (definition.bitWidth - 8)) & mask;
        for (int bitIndex = 0; bitIndex < 8; ++bitIndex) {
            const bool carry = (crc & topBit) != 0;
            crc = (crc << 1) & mask;
            if (carry) {
                crc ^= definition.polynomial;
            }
        }
    }

    if (definition.reflectOutput) {
        crc = reflectBits(crc, definition.bitWidth);
    }

    crc ^= definition.xorOutput;
    return crc & mask;
}

QByteArray buildChecksumBytes(const PacketChecksumDef &definition, const QByteArray &bytes)
{
    if (definition.bitWidth == 0) {
        return {};
    }

    if (definition.sum8) {
        quint32 total = 0;
        for (const char byte : bytes) {
            total += static_cast<quint8>(byte);
        }
        return QByteArray(1, static_cast<char>(total & 0xFF));
    }

    const quint64 crc = computeCrc(definition, bytes);
    return encodeUnsignedInteger(crc, definition.bitWidth / 8, definition.littleEndian);
}

QByteArray encodeFieldValue(const PacketFieldDef &field,
                            const QString &input,
                            QString *errorMessage)
{
    auto setError = [errorMessage, &field](const QString &details) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Field \"%1\": %2").arg(field.name, details);
        }
    };

    switch (field.type) {
    case PacketValueType::UInt: {
        quint64 raw = 0;
        QString parseError;
        if (!parseUnsignedFieldInput(field,
                                     input,
                                     unsignedMaximumForWidth(field.byteWidth),
                                     &raw,
                                     &parseError)) {
            setError(parseError);
            return {};
        }
        return encodeUnsignedInteger(raw, field.byteWidth, field.endian != QStringLiteral("big"));
    }
    case PacketValueType::Int: {
        qint64 raw = 0;
        QString parseError;
        if (!parseSignedFieldInput(field,
                                   input,
                                   signedMinimumForWidth(field.byteWidth),
                                   signedMaximumForWidth(field.byteWidth),
                                   &raw,
                                   &parseError)) {
            setError(parseError);
            return {};
        }

        quint64 rawBits = static_cast<quint64>(raw);
        if (field.byteWidth < 8) {
            rawBits &= maskForWidth(field.byteWidth);
        }
        return encodeUnsignedInteger(rawBits, field.byteWidth, field.endian != QStringLiteral("big"));
    }
    case PacketValueType::Float: {
        bool ok = false;
        const double displayValue = input.toDouble(&ok);
        if (!ok) {
            setError(QStringLiteral("invalid %1.").arg(typeLabel(field)));
            return {};
        }

        const double rawValue = displayValue / field.scale;
        if (!std::isfinite(rawValue)) {
            setError(QStringLiteral("%1 is out of range.").arg(typeLabel(field)));
            return {};
        }

        if (field.byteWidth == 4) {
            const float rawFloat = static_cast<float>(rawValue);
            quint32 rawBits = 0;
            std::memcpy(&rawBits, &rawFloat, sizeof(rawBits));
            return encodeUnsignedInteger(rawBits, 4, field.endian != QStringLiteral("big"));
        }

        if (field.byteWidth == 8) {
            quint64 rawBits = 0;
            std::memcpy(&rawBits, &rawValue, sizeof(rawBits));
            return encodeUnsignedInteger(rawBits, 8, field.endian != QStringLiteral("big"));
        }

        setError(QStringLiteral("unsupported float byte width."));
        return {};
    }
    case PacketValueType::Unknown:
    default:
        setError(QStringLiteral("unsupported field type."));
        return {};
    }
}

bool decodeFieldValue(const PacketFieldDef &field,
                      const QByteArray &bytes,
                      double *numericValue,
                      QString *displayValue,
                      QString *errorMessage)
{
    auto setError = [errorMessage, &field](const QString &details) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Field \"%1\": %2").arg(field.name, details);
        }
    };

    if (bytes.size() != packetFieldWidth(field)) {
        setError(QStringLiteral("field width mismatch."));
        return false;
    }

    const bool littleEndian = field.endian != QStringLiteral("big");
    double value = 0.0;

    switch (field.type) {
    case PacketValueType::UInt: {
        const quint64 raw = decodeUnsignedInteger(bytes, littleEndian);
        value = static_cast<double>(raw) * field.scale;
        if (displayValue != nullptr) {
            *displayValue = formatIntegerDisplay(raw, field);
        }
        break;
    }
    case PacketValueType::Int: {
        quint64 rawBits = decodeUnsignedInteger(bytes, littleEndian);
        if (field.byteWidth < 8) {
            const quint64 signBit = quint64{1} << ((field.byteWidth * 8) - 1);
            if ((rawBits & signBit) != 0) {
                rawBits |= ~maskForWidth(field.byteWidth);
            }
        }

        const qint64 raw = static_cast<qint64>(rawBits);
        value = static_cast<double>(raw) * field.scale;
        if (displayValue != nullptr) {
            *displayValue = formatSignedDisplay(raw, field);
        }
        break;
    }
    case PacketValueType::Float: {
        const quint64 rawBits = decodeUnsignedInteger(bytes, littleEndian);
        if (field.byteWidth == 4) {
            const quint32 floatBits = static_cast<quint32>(rawBits);
            float rawFloat = 0.0f;
            std::memcpy(&rawFloat, &floatBits, sizeof(rawFloat));
            value = static_cast<double>(rawFloat) * field.scale;
        } else if (field.byteWidth == 8) {
            double rawDouble = 0.0;
            std::memcpy(&rawDouble, &rawBits, sizeof(rawDouble));
            value = rawDouble * field.scale;
        } else {
            setError(QStringLiteral("unsupported float byte width."));
            return false;
        }
        break;
    }
    case PacketValueType::Unknown:
    default:
        setError(QStringLiteral("unsupported field type."));
        return false;
    }

    if (numericValue != nullptr) {
        *numericValue = value;
    }
    if (displayValue != nullptr && displayValue->isEmpty()) {
        *displayValue = formatDisplayValue(value, field);
    }
    return true;
}

} // namespace

bool PacketSchema::isValid() const
{
    return !name.isEmpty()
        && !header.isEmpty()
        && !fields.isEmpty()
        && isSupportedPacketChecksum(checksum);
}

int PacketSchema::checksumSize() const
{
    return packetChecksumSize(checksum);
}

int PacketSchema::payloadSize() const
{
    int total = 0;
    for (const PacketFieldDef &field : fields) {
        total += packetFieldWidth(field);
    }
    return total;
}

int PacketSchema::frameSize() const
{
    return header.size() + payloadSize() + checksumSize() + footer.size();
}

QStringList PacketSchema::chartFieldNames() const
{
    QStringList names;
    for (const PacketFieldDef &field : fields) {
        if (field.chart) {
            names.append(field.name);
        }
    }
    return names;
}

PacketValueType packetValueTypeFromName(const QString &typeName)
{
    PacketValueType type = PacketValueType::Unknown;
    if (resolvePacketFieldType(typeName, 0, &type, nullptr)) {
        return type;
    }
    return PacketValueType::Unknown;
}

QString packetValueTypeName(PacketValueType type)
{
    switch (type) {
    case PacketValueType::UInt:
        return QStringLiteral("uint");
    case PacketValueType::Int:
        return QStringLiteral("int");
    case PacketValueType::Float:
        return QStringLiteral("float");
    case PacketValueType::Unknown:
    default:
        return QStringLiteral("unknown");
    }
}

bool normalizePacketFieldDef(PacketFieldDef *field, QString *errorMessage)
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    if (field == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Packet field definition is null.");
        }
        return false;
    }

    PacketValueType resolvedType = PacketValueType::Unknown;
    int resolvedWidth = 0;
    if (!resolvePacketFieldType(field->typeName, field->byteWidth, &resolvedType, &resolvedWidth)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Unsupported field type: %1").arg(field->typeName);
        }
        return false;
    }

    if (!isValidByteWidth(resolvedType, resolvedWidth)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Invalid byte width %1 for type %2.")
                                .arg(resolvedWidth)
                                .arg(packetValueTypeName(resolvedType));
        }
        return false;
    }

    field->type = resolvedType;
    field->typeName = packetValueTypeName(resolvedType);
    field->byteWidth = resolvedWidth;

    if (field->endian.isEmpty()) {
        field->endian = QStringLiteral("little");
    } else {
        field->endian = field->endian.trimmed().toLower();
    }

    if (field->endian != QStringLiteral("little") && field->endian != QStringLiteral("big")) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Unsupported endian: %1").arg(field->endian);
        }
        return false;
    }

    if (field->precision < 0) {
        field->precision = (field->type == PacketValueType::Float || field->scale < 1.0) ? 2 : 0;
    }

    if (field->defaultValue.isEmpty()) {
        field->defaultValue = QStringLiteral("0");
    }

    return true;
}

int packetFieldWidth(const PacketFieldDef &field)
{
    return field.byteWidth;
}

bool isSupportedPacketChecksum(const QString &checksumName)
{
    return packetChecksumDefinition(checksumName) != nullptr;
}

int packetChecksumSize(const QString &checksumName)
{
    const PacketChecksumDef *definition = packetChecksumDefinition(checksumName);
    if (definition == nullptr) {
        return -1;
    }
    return definition->bitWidth / 8;
}

QByteArray buildPacketChecksumBytes(const QString &checksumName,
                                    const QByteArray &bytes,
                                    QString *errorMessage)
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    const PacketChecksumDef *definition = packetChecksumDefinition(checksumName);
    if (definition == nullptr) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Unsupported checksum type: %1").arg(checksumName);
        }
        return {};
    }

    return buildChecksumBytes(*definition, bytes);
}

bool loadPacketSchemaFromJson(const QByteArray &jsonBytes,
                              PacketSchema *schema,
                              QString *errorMessage)
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    const QJsonDocument document = QJsonDocument::fromJson(jsonBytes);
    if (!document.isObject()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Schema JSON root must be an object.");
        }
        return false;
    }

    const QJsonObject root = document.object();
    PacketSchema parsedSchema;
    parsedSchema.name = root.value(QStringLiteral("name")).toString();
    parsedSchema.checksum = root.value(QStringLiteral("checksum")).toString(QStringLiteral("sum8")).trimmed().toLower();

    parsedSchema.header = parseHexBytes(root.value(QStringLiteral("header")).toString(),
                                        QStringLiteral("Packet header"),
                                        errorMessage);
    if (parsedSchema.header.isEmpty()) {
        return false;
    }

    const QString footerText = root.value(QStringLiteral("footer")).toString();
    if (!footerText.trimmed().isEmpty()) {
        parsedSchema.footer = parseHexBytes(footerText, QStringLiteral("Packet footer"), errorMessage);
        if (parsedSchema.footer.isEmpty()) {
            return false;
        }
    }

    const QJsonArray fieldArray = root.value(QStringLiteral("fields")).toArray();
    if (fieldArray.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Schema must contain at least one field.");
        }
        return false;
    }

    parsedSchema.fields.reserve(fieldArray.size());
    for (const QJsonValue &value : fieldArray) {
        const QJsonObject fieldObject = value.toObject();
        PacketFieldDef field;
        field.name = fieldObject.value(QStringLiteral("name")).toString();
        field.typeName = fieldObject.value(QStringLiteral("type")).toString(QStringLiteral("uint")).trimmed().toLower();
        field.byteWidth = fieldObject.value(QStringLiteral("byteWidth"))
                              .toInt(fieldObject.value(QStringLiteral("bytes")).toInt(0));
        field.endian = fieldObject.value(QStringLiteral("endian")).toString(QStringLiteral("little")).trimmed().toLower();
        field.scale = fieldObject.value(QStringLiteral("scale")).toDouble(1.0);
        field.precision = fieldObject.contains(QStringLiteral("precision"))
            ? fieldObject.value(QStringLiteral("precision")).toInt()
            : -1;
        field.unit = fieldObject.value(QStringLiteral("unit")).toString();
        field.chart = fieldObject.value(QStringLiteral("chart")).toBool(false);
        field.defaultValue = fieldObject.value(QStringLiteral("defaultValue")).toString(QStringLiteral("0"));

        if (field.name.isEmpty()) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Schema field is missing a name.");
            }
            return false;
        }

        QString fieldError;
        if (!normalizePacketFieldDef(&field, &fieldError)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Schema field \"%1\" is invalid: %2")
                                    .arg(field.name, fieldError);
            }
            return false;
        }

        if (qFuzzyIsNull(field.scale)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Schema field \"%1\" cannot use scale 0.").arg(field.name);
            }
            return false;
        }

        parsedSchema.fields.append(field);
    }

    if (parsedSchema.name.isEmpty()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Schema must have a name.");
        }
        return false;
    }

    if (!isSupportedPacketChecksum(parsedSchema.checksum)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Unsupported checksum type: %1").arg(parsedSchema.checksum);
        }
        return false;
    }

    if (schema != nullptr) {
        *schema = parsedSchema;
    }
    return true;
}

bool loadPacketSchemaFromFile(const QString &path,
                              PacketSchema *schema,
                              QString *errorMessage)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Failed to open schema: %1").arg(file.errorString());
        }
        return false;
    }

    return loadPacketSchemaFromJson(file.readAll(), schema, errorMessage);
}

bool buildPacketSchema(const QString &name,
                       const QString &headerText,
                       const QString &footerText,
                       const QString &checksum,
                       const QVector<PacketFieldDef> &fields,
                       PacketSchema *schema,
                       QString *errorMessage)
{
    QJsonArray fieldArray;

    for (PacketFieldDef field : fields) {
        QString fieldError;
        if (!normalizePacketFieldDef(&field, &fieldError)) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Field \"%1\": %2").arg(field.name, fieldError);
            }
            return false;
        }

        fieldArray.append(QJsonObject{
            {QStringLiteral("name"), field.name},
            {QStringLiteral("type"), field.typeName},
            {QStringLiteral("byteWidth"), field.byteWidth},
            {QStringLiteral("endian"), field.endian},
            {QStringLiteral("scale"), field.scale},
            {QStringLiteral("precision"), field.precision},
            {QStringLiteral("unit"), field.unit},
            {QStringLiteral("chart"), field.chart},
            {QStringLiteral("defaultValue"), field.defaultValue},
        });
    }

    const QJsonObject root{
        {QStringLiteral("name"), name},
        {QStringLiteral("header"), headerText},
        {QStringLiteral("footer"), footerText},
        {QStringLiteral("checksum"), checksum},
        {QStringLiteral("fields"), fieldArray},
    };

    return loadPacketSchemaFromJson(QJsonDocument(root).toJson(QJsonDocument::Compact),
                                    schema,
                                    errorMessage);
}

QString packetHeaderText(const QByteArray &header)
{
    if (header.isEmpty()) {
        return {};
    }

    return QString::fromLatin1(header.toHex(' ')).toUpper();
}

QJsonObject packetSchemaToJsonObject(const PacketSchema &schema)
{
    QJsonArray fieldArray;

    for (const PacketFieldDef &field : schema.fields) {
        fieldArray.append(QJsonObject{
            {QStringLiteral("name"), field.name},
            {QStringLiteral("type"), field.typeName},
            {QStringLiteral("byteWidth"), field.byteWidth},
            {QStringLiteral("endian"), field.endian},
            {QStringLiteral("scale"), field.scale},
            {QStringLiteral("precision"), field.precision},
            {QStringLiteral("unit"), field.unit},
            {QStringLiteral("chart"), field.chart},
            {QStringLiteral("defaultValue"), field.defaultValue},
        });
    }

    return QJsonObject{
        {QStringLiteral("name"), schema.name},
        {QStringLiteral("header"), packetHeaderText(schema.header)},
        {QStringLiteral("footer"), packetHeaderText(schema.footer)},
        {QStringLiteral("checksum"), schema.checksum},
        {QStringLiteral("fields"), fieldArray},
    };
}

QByteArray buildPacketFrame(const PacketSchema &schema,
                            const QStringList &fieldValues,
                            QString *errorMessage)
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    if (!schema.isValid()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No packet schema is loaded.");
        }
        return {};
    }

    if (fieldValues.size() != schema.fields.size()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Packet field value count does not match the schema.");
        }
        return {};
    }

    QByteArray frame = schema.header;
    for (int index = 0; index < schema.fields.size(); ++index) {
        const QByteArray encoded = encodeFieldValue(schema.fields.at(index), fieldValues.at(index), errorMessage);
        if (encoded.isEmpty()) {
            return {};
        }
        frame.append(encoded);
    }

    const QByteArray checksumBytes = buildPacketChecksumBytes(schema.checksum, frame, errorMessage);
    if (packetChecksumSize(schema.checksum) > 0 && checksumBytes.size() != packetChecksumSize(schema.checksum)) {
        return {};
    }

    frame.append(checksumBytes);
    frame.append(schema.footer);
    return frame;
}

QString buildPacketHexPreview(const PacketSchema &schema,
                              const QStringList &fieldValues,
                              QString *errorMessage)
{
    const QByteArray frame = buildPacketFrame(schema, fieldValues, errorMessage);
    if (frame.isEmpty()) {
        return {};
    }

    QStringList hexBytes;
    hexBytes.reserve(frame.size());
    for (const char byte : frame) {
        hexBytes.append(QStringLiteral("%1")
                            .arg(static_cast<quint8>(byte), 2, 16, QLatin1Char('0'))
                            .toUpper());
    }
    return hexBytes.join(QChar(' '));
}

bool parsePacketFrame(const PacketSchema &schema,
                      const QByteArray &frame,
                      QVector<double> *numericValues,
                      QStringList *displayValues,
                      QString *errorMessage)
{
    if (errorMessage != nullptr) {
        errorMessage->clear();
    }

    if (!schema.isValid()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("No packet schema is loaded.");
        }
        return false;
    }

    if (frame.size() != schema.frameSize()) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Frame size does not match schema size.");
        }
        return false;
    }

    if (!frame.startsWith(schema.header)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Frame header does not match schema header.");
        }
        return false;
    }

    if (!schema.footer.isEmpty() && !frame.endsWith(schema.footer)) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Frame footer does not match schema footer.");
        }
        return false;
    }

    const int checksumSize = schema.checksumSize();
    if (checksumSize < 0) {
        if (errorMessage != nullptr) {
            *errorMessage = QStringLiteral("Unsupported checksum type: %1").arg(schema.checksum);
        }
        return false;
    }

    if (checksumSize > 0) {
        const int checksumIndex = frame.size() - schema.footer.size() - checksumSize;
        QString checksumError;
        QString *checksumErrorPtr = errorMessage != nullptr ? errorMessage : &checksumError;
        const QByteArray expected = buildPacketChecksumBytes(schema.checksum,
                                                             frame.left(checksumIndex),
                                                             checksumErrorPtr);
        if (!checksumErrorPtr->isEmpty()) {
            return false;
        }

        const QByteArray actual = frame.mid(checksumIndex, checksumSize);
        if (expected != actual) {
            if (errorMessage != nullptr) {
                *errorMessage = QStringLiteral("Checksum mismatch: expected %1, got %2.")
                                    .arg(QString::fromLatin1(expected.toHex(' ')).toUpper(),
                                         QString::fromLatin1(actual.toHex(' ')).toUpper());
            }
            return false;
        }
    }

    QVector<double> parsedValues;
    parsedValues.reserve(schema.fields.size());
    QStringList parsedDisplayValues;
    parsedDisplayValues.reserve(schema.fields.size());

    int offset = schema.header.size();
    for (const PacketFieldDef &field : schema.fields) {
        const int width = packetFieldWidth(field);
        const QByteArray fieldBytes = frame.mid(offset, width);

        double numericValue = 0.0;
        QString displayValue;
        if (!decodeFieldValue(field, fieldBytes, &numericValue, &displayValue, errorMessage)) {
            return false;
        }

        parsedValues.append(numericValue);
        parsedDisplayValues.append(displayValue);
        offset += width;
    }

    if (numericValues != nullptr) {
        *numericValues = parsedValues;
    }
    if (displayValues != nullptr) {
        *displayValues = parsedDisplayValues;
    }
    return true;
}
