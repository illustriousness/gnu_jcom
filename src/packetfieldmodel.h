#pragma once

#include "packetschema.h"

#include <QAbstractListModel>

class PacketFieldModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit PacketFieldModel(QObject *parent = nullptr);

    enum Roles {
        NameRole = Qt::UserRole + 1,
        TypeRole,
        ByteWidthRole,
        UnitRole,
        EndianRole,
        ChartRole,
        SendValueRole,
        ReceivedValueRole,
        PrecisionRole,
        ScaleRole,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    void clear();
    void setSchema(const PacketSchema &schema);
    void setFieldDefinitions(const QVector<PacketFieldDef> &fields);
    QVector<PacketFieldDef> fieldDefinitions() const;
    QStringList sendValues() const;
    void setSendValues(const QStringList &values);
    void updateReceivedValues(const QStringList &values);
    QStringList chartSeriesNames() const;

    Q_INVOKABLE bool appendField(const QString &name = QString());
    Q_INVOKABLE bool removeField(int index);
    Q_INVOKABLE bool moveFieldUp(int index);
    Q_INVOKABLE bool moveFieldDown(int index);
    Q_INVOKABLE bool setFieldName(int index, const QString &name);
    Q_INVOKABLE bool setFieldType(int index, const QString &typeName);
    Q_INVOKABLE bool setFieldByteWidth(int index, int byteWidth);
    Q_INVOKABLE bool setFieldEndian(int index, const QString &endian);
    Q_INVOKABLE bool setFieldScale(int index, double scale);
    Q_INVOKABLE bool setFieldUnit(int index, const QString &unit);
    Q_INVOKABLE bool setFieldChart(int index, bool chart);
    Q_INVOKABLE bool setSendValue(int index, const QString &value);
    Q_INVOKABLE QVariantMap get(int index) const;

signals:
    void countChanged();

private:
    struct FieldState {
        PacketFieldDef def;
        QString sendValue;
        QString receivedValue;
    };

    QVector<FieldState> m_fields;
};
