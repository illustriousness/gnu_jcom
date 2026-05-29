#pragma once

#include <QAbstractListModel>
#include <QVector>

class LogModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit LogModel(QObject *parent = nullptr);

    enum Roles {
        TimestampRole = Qt::UserRole + 1,
        KindRole,
        MessageRole,
        HexRole,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QString toPlainText() const;

    void append(const QString &kind, const QString &text, const QString &hex = QString());

    Q_INVOKABLE void clear();

signals:
    void countChanged();

private:
    struct Entry {
        QString timestamp;
        QString kind;
        QString text;
        QString hex;
    };

    void trimIfNeeded();

    QVector<Entry> m_entries;
    int m_maxEntries = 2000;
};
