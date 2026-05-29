#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVector>

enum class PacketValueType {
    Unknown = 0,
    UInt,
    Int,
    Float,
};

struct PacketFieldDef {
    QString name;
    PacketValueType type = PacketValueType::Unknown;
    QString typeName;
    int byteWidth = 0;
    QString endian = QStringLiteral("little");
    double scale = 1.0;
    int precision = 2;
    QString unit;
    bool chart = false;
    QString defaultValue = QStringLiteral("0");
};

struct PacketSchema {
    QString name;
    QByteArray header;
    QByteArray footer;
    QString checksum = QStringLiteral("sum8");
    int checksumStart = 0;
    QVector<PacketFieldDef> fields;

    bool isValid() const;
    int checksumSize() const;
    int payloadSize() const;
    int frameSize() const;
    QStringList chartFieldNames() const;
};

PacketValueType packetValueTypeFromName(const QString &typeName);
QString packetValueTypeName(PacketValueType type);
bool normalizePacketFieldDef(PacketFieldDef *field, QString *errorMessage = nullptr);
int packetFieldWidth(const PacketFieldDef &field);
bool isSupportedPacketChecksum(const QString &checksumName);
int packetChecksumSize(const QString &checksumName);
QByteArray buildPacketChecksumBytes(const QString &checksumName,
                                    const QByteArray &bytes,
                                    QString *errorMessage = nullptr);

bool loadPacketSchemaFromJson(const QByteArray &jsonBytes,
                              PacketSchema *schema,
                              QString *errorMessage = nullptr);
bool loadPacketSchemaFromFile(const QString &path,
                              PacketSchema *schema,
                              QString *errorMessage = nullptr);
bool buildPacketSchema(const QString &name,
                       const QString &headerText,
                       const QString &footerText,
                       const QString &checksum,
                       int checksumStart,
                       const QVector<PacketFieldDef> &fields,
                       PacketSchema *schema,
                       QString *errorMessage = nullptr);
QString packetHeaderText(const QByteArray &header);
QJsonObject packetSchemaToJsonObject(const PacketSchema &schema);

QByteArray buildPacketFrame(const PacketSchema &schema,
                            const QStringList &fieldValues,
                            QString *errorMessage = nullptr);
QString buildPacketHexPreview(const PacketSchema &schema,
                              const QStringList &fieldValues,
                              QString *errorMessage = nullptr);
bool parsePacketFrame(const PacketSchema &schema,
                      const QByteArray &frame,
                      QVector<double> *numericValues,
                      QStringList *displayValues,
                      QString *errorMessage = nullptr);
