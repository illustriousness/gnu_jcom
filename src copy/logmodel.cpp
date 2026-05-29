#include "logmodel.h"

#include <QDateTime>
#include <QStringBuilder>
#include <QStringList>

LogModel::LogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int LogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_entries.size();
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size()) {
        return {};
    }

    const Entry &entry = m_entries.at(index.row());
    switch (role) {
    case TimestampRole:
        return entry.timestamp;
    case KindRole:
        return entry.kind;
    case MessageRole:
        return entry.text;
    case HexRole:
        return entry.hex;
    default:
        return {};
    }
}

QHash<int, QByteArray> LogModel::roleNames() const
{
    return {
        {TimestampRole, "timestamp"},
        {KindRole, "kind"},
        {MessageRole, "message"},
        {HexRole, "hex"},
    };
}

int LogModel::count() const
{
    return m_entries.size();
}

QString LogModel::toPlainText() const
{
    QStringList lines;
    lines.reserve(m_entries.size());

    for (const Entry &entry : m_entries) {
        QString line = QStringLiteral("[%1] %2: %3")
                           .arg(entry.timestamp, entry.kind.toUpper(), entry.text);
        if (!entry.hex.isEmpty() && entry.hex != entry.text) {
            line += QStringLiteral(" | HEX %1").arg(entry.hex);
        }
        lines.append(line);
    }

    return lines.join(QChar('\n'));
}

void LogModel::append(const QString &kind, const QString &text, const QString &hex)
{
    const int insertIndex = m_entries.size();
    beginInsertRows(QModelIndex(), insertIndex, insertIndex);
    m_entries.append(Entry{
        .timestamp = QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss.zzz")),
        .kind = kind,
        .text = text,
        .hex = hex,
    });
    endInsertRows();
    emit countChanged();

    trimIfNeeded();
}

void LogModel::clear()
{
    if (m_entries.isEmpty()) {
        return;
    }

    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

void LogModel::trimIfNeeded()
{
    while (m_entries.size() > m_maxEntries) {
        beginRemoveRows(QModelIndex(), 0, 0);
        m_entries.removeFirst();
        endRemoveRows();
        emit countChanged();
    }
}
