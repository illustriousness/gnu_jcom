#include "packetfieldmodel.h"

namespace {

PacketFieldDef defaultFieldDefinition(const QString &name)
{
    PacketFieldDef field;
    field.name = name;
    field.type = PacketValueType::UInt;
    field.typeName = QStringLiteral("uint");
    field.byteWidth = 2;
    field.endian = QStringLiteral("little");
    field.scale = 1.0;
    field.precision = 0;
    field.defaultValue = QStringLiteral("0");
    return field;
}

} // namespace

PacketFieldModel::PacketFieldModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int PacketFieldModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_fields.size();
}

QVariant PacketFieldModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_fields.size()) {
        return {};
    }

    const FieldState &field = m_fields.at(index.row());
    switch (role) {
    case NameRole:
        return field.def.name;
    case TypeRole:
        return field.def.typeName;
    case ByteWidthRole:
        return field.def.byteWidth;
    case UnitRole:
        return field.def.unit;
    case EndianRole:
        return field.def.endian;
    case ChartRole:
        return field.def.chart;
    case SendValueRole:
        return field.sendValue;
    case ReceivedValueRole:
        return field.receivedValue;
    case PrecisionRole:
        return field.def.precision;
    case ScaleRole:
        return field.def.scale;
    default:
        return {};
    }
}

QHash<int, QByteArray> PacketFieldModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {TypeRole, "typeName"},
        {ByteWidthRole, "byteWidth"},
        {UnitRole, "unit"},
        {EndianRole, "endian"},
        {ChartRole, "chart"},
        {SendValueRole, "sendValue"},
        {ReceivedValueRole, "receivedValue"},
        {PrecisionRole, "precision"},
        {ScaleRole, "scale"},
    };
}

int PacketFieldModel::count() const
{
    return m_fields.size();
}

void PacketFieldModel::clear()
{
    if (m_fields.isEmpty()) {
        return;
    }

    beginResetModel();
    m_fields.clear();
    endResetModel();
    emit countChanged();
}

void PacketFieldModel::setSchema(const PacketSchema &schema)
{
    setFieldDefinitions(schema.fields);
}

void PacketFieldModel::setFieldDefinitions(const QVector<PacketFieldDef> &fields)
{
    beginResetModel();
    m_fields.clear();
    m_fields.reserve(fields.size());

    for (const PacketFieldDef &field : fields) {
        PacketFieldDef normalizedField = field;
        if (!normalizePacketFieldDef(&normalizedField, nullptr)) {
            normalizedField = defaultFieldDefinition(normalizedField.name);
        }

        m_fields.append(FieldState{
            .def = normalizedField,
            .sendValue = normalizedField.defaultValue,
            .receivedValue = QStringLiteral("-"),
        });
    }

    endResetModel();
    emit countChanged();
}

QVector<PacketFieldDef> PacketFieldModel::fieldDefinitions() const
{
    QVector<PacketFieldDef> fields;
    fields.reserve(m_fields.size());
    for (const FieldState &fieldState : m_fields) {
        PacketFieldDef field = fieldState.def;
        field.defaultValue = fieldState.sendValue;
        fields.append(field);
    }
    return fields;
}

QStringList PacketFieldModel::sendValues() const
{
    QStringList values;
    values.reserve(m_fields.size());
    for (const FieldState &field : m_fields) {
        values.append(field.sendValue);
    }
    return values;
}

void PacketFieldModel::setSendValues(const QStringList &values)
{
    const int countToUpdate = qMin(values.size(), m_fields.size());
    for (int index = 0; index < countToUpdate; ++index) {
        m_fields[index].sendValue = values.at(index);
        const QModelIndex modelIndex = this->index(index, 0);
        emit dataChanged(modelIndex, modelIndex, {SendValueRole});
    }
}

void PacketFieldModel::updateReceivedValues(const QStringList &values)
{
    const int countToUpdate = qMin(values.size(), m_fields.size());
    for (int index = 0; index < countToUpdate; ++index) {
        m_fields[index].receivedValue = values.at(index);
        const QModelIndex modelIndex = this->index(index, 0);
        emit dataChanged(modelIndex, modelIndex, {ReceivedValueRole});
    }
}

QStringList PacketFieldModel::chartSeriesNames() const
{
    QStringList names;
    for (const FieldState &field : m_fields) {
        if (field.def.chart) {
            names.append(field.def.name);
        }
    }
    return names;
}

bool PacketFieldModel::appendField(const QString &name)
{
    QString fieldName = name.trimmed();
    if (fieldName.isEmpty()) {
        fieldName = QStringLiteral("field%1").arg(m_fields.size() + 1);
    }

    const int insertIndex = m_fields.size();
    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    m_fields.append(FieldState{
        .def = defaultFieldDefinition(fieldName),
        .sendValue = QStringLiteral("0"),
        .receivedValue = QStringLiteral("-"),
    });
    endInsertRows();
    emit countChanged();
    return true;
}

bool PacketFieldModel::removeField(int index)
{
    if (index < 0 || index >= m_fields.size()) {
        return false;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_fields.removeAt(index);
    endRemoveRows();
    emit countChanged();
    return true;
}

bool PacketFieldModel::moveFieldUp(int index)
{
    if (index <= 0 || index >= m_fields.size()) {
        return false;
    }

    beginMoveRows(QModelIndex(), index, index, QModelIndex(), index - 1);
    m_fields.move(index, index - 1);
    endMoveRows();
    return true;
}

bool PacketFieldModel::moveFieldDown(int index)
{
    if (index < 0 || index >= m_fields.size() - 1) {
        return false;
    }

    beginMoveRows(QModelIndex(), index, index, QModelIndex(), index + 2);
    m_fields.move(index, index + 1);
    endMoveRows();
    return true;
}

bool PacketFieldModel::setFieldName(int index, const QString &name)
{
    if (index < 0 || index >= m_fields.size()) {
        return false;
    }

    const QString normalized = name.trimmed();
    if (normalized.isEmpty() || m_fields[index].def.name == normalized) {
        return !normalized.isEmpty();
    }

    m_fields[index].def.name = normalized;
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {NameRole});
    return true;
}

bool PacketFieldModel::setFieldType(int index, const QString &typeName)
{
    if (index < 0 || index >= m_fields.size()) {
        return false;
    }

    PacketFieldDef nextField = m_fields[index].def;
    nextField.typeName = typeName.trimmed().toLower();
    if (!normalizePacketFieldDef(&nextField, nullptr)) {
        return false;
    }

    if (m_fields[index].def.type == nextField.type
        && m_fields[index].def.typeName == nextField.typeName
        && m_fields[index].def.byteWidth == nextField.byteWidth) {
        return true;
    }

    m_fields[index].def = nextField;
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {TypeRole, ByteWidthRole, EndianRole, PrecisionRole});
    return true;
}

bool PacketFieldModel::setFieldByteWidth(int index, int byteWidth)
{
    if (index < 0 || index >= m_fields.size()) {
        return false;
    }

    PacketFieldDef nextField = m_fields[index].def;
    nextField.byteWidth = byteWidth;
    if (!normalizePacketFieldDef(&nextField, nullptr)) {
        return false;
    }

    if (m_fields[index].def.byteWidth == nextField.byteWidth) {
        return true;
    }

    m_fields[index].def = nextField;
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {ByteWidthRole, EndianRole, PrecisionRole});
    return true;
}

bool PacketFieldModel::setFieldEndian(int index, const QString &endian)
{
    if (index < 0 || index >= m_fields.size()) {
        return false;
    }

    const QString normalized = endian.trimmed().toLower();
    if (normalized != QStringLiteral("little") && normalized != QStringLiteral("big")) {
        return false;
    }

    if (m_fields[index].def.endian == normalized) {
        return true;
    }

    m_fields[index].def.endian = normalized;
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {EndianRole});
    return true;
}

bool PacketFieldModel::setFieldScale(int index, double scale)
{
    if (index < 0 || index >= m_fields.size() || qFuzzyIsNull(scale)) {
        return false;
    }

    if (qFuzzyCompare(m_fields[index].def.scale, scale)) {
        return true;
    }

    m_fields[index].def.scale = scale;
    m_fields[index].def.precision = scale < 1.0 ? 2 : 0;
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {ScaleRole, PrecisionRole});
    return true;
}

bool PacketFieldModel::setFieldUnit(int index, const QString &unit)
{
    if (index < 0 || index >= m_fields.size()) {
        return false;
    }

    if (m_fields[index].def.unit == unit) {
        return true;
    }

    m_fields[index].def.unit = unit;
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {UnitRole});
    return true;
}

bool PacketFieldModel::setFieldChart(int index, bool chart)
{
    if (index < 0 || index >= m_fields.size()) {
        return false;
    }

    if (m_fields[index].def.chart == chart) {
        return true;
    }

    m_fields[index].def.chart = chart;
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {ChartRole});
    return true;
}

bool PacketFieldModel::setSendValue(int index, const QString &value)
{
    if (index < 0 || index >= m_fields.size()) {
        return false;
    }

    if (m_fields[index].sendValue == value) {
        return true;
    }

    m_fields[index].sendValue = value;
    m_fields[index].def.defaultValue = value;
    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {SendValueRole});
    return true;
}

QVariantMap PacketFieldModel::get(int index) const
{
    if (index < 0 || index >= m_fields.size()) {
        return {};
    }

    const FieldState &field = m_fields.at(index);
    return {
        {QStringLiteral("name"), field.def.name},
        {QStringLiteral("typeName"), field.def.typeName},
        {QStringLiteral("byteWidth"), field.def.byteWidth},
        {QStringLiteral("unit"), field.def.unit},
        {QStringLiteral("endian"), field.def.endian},
        {QStringLiteral("chart"), field.def.chart},
        {QStringLiteral("sendValue"), field.sendValue},
        {QStringLiteral("receivedValue"), field.receivedValue},
        {QStringLiteral("precision"), field.def.precision},
        {QStringLiteral("scale"), field.def.scale},
    };
}
