#include "sendlistmodel.h"

#include <QJsonObject>

SendListModel::SendListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SendListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_items.size();
}

QVariant SendListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size()) {
        return {};
    }

    const Item &item = m_items.at(index.row());
    switch (role) {
    case NameRole:
        return item.name;
    case PayloadRole:
        return item.payload;
    case ModeRole:
        return item.mode;
    default:
        return {};
    }
}

QHash<int, QByteArray> SendListModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {PayloadRole, "payload"},
        {ModeRole, "mode"},
    };
}

int SendListModel::count() const
{
    return m_items.size();
}

QJsonArray SendListModel::toJson() const
{
    QJsonArray array;

    for (const Item &item : m_items) {
        array.append(QJsonObject{
            {QStringLiteral("name"), item.name},
            {QStringLiteral("payload"), item.payload},
            {QStringLiteral("mode"), item.mode},
        });
    }

    return array;
}

void SendListModel::fromJson(const QJsonArray &array)
{
    beginResetModel();
    m_items.clear();
    m_items.reserve(array.size());

    for (const QJsonValue &value : array) {
        const QJsonObject object = value.toObject();
        m_items.append(Item{
            .name = object.value(QStringLiteral("name")).toString(),
            .payload = object.value(QStringLiteral("payload")).toString(),
            .mode = object.value(QStringLiteral("mode")).toInt(),
        });
    }

    endResetModel();
    emit countChanged();
}

QVariantMap SendListModel::get(int index) const
{
    if (index < 0 || index >= m_items.size()) {
        return {};
    }

    const Item &item = m_items.at(index);
    return {
        {QStringLiteral("name"), item.name},
        {QStringLiteral("payload"), item.payload},
        {QStringLiteral("mode"), item.mode},
    };
}

bool SendListModel::appendItem(const QString &name, const QString &payload, int mode)
{
    const int insertIndex = m_items.size();
    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    m_items.append(Item{
        .name = name,
        .payload = payload,
        .mode = mode,
    });
    endInsertRows();
    emit countChanged();
    return true;
}

bool SendListModel::updateItem(int index, const QString &name, const QString &payload, int mode)
{
    if (index < 0 || index >= m_items.size()) {
        return false;
    }

    Item &item = m_items[index];
    item.name = name;
    item.payload = payload;
    item.mode = mode;

    const QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex, {NameRole, PayloadRole, ModeRole});
    return true;
}

bool SendListModel::removeItem(int index)
{
    if (index < 0 || index >= m_items.size()) {
        return false;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_items.removeAt(index);
    endRemoveRows();
    emit countChanged();
    return true;
}
