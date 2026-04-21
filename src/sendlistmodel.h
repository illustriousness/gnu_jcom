#pragma once

#include <QAbstractListModel>
#include <QJsonArray>
#include <QVector>

class SendListModel final : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    explicit SendListModel(QObject *parent = nullptr);

    enum Roles {
        NameRole = Qt::UserRole + 1,
        PayloadRole,
        ModeRole,
    };

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    int count() const;
    QJsonArray toJson() const;
    void fromJson(const QJsonArray &array);

    Q_INVOKABLE QVariantMap get(int index) const;

    bool appendItem(const QString &name, const QString &payload, int mode);
    bool updateItem(int index, const QString &name, const QString &payload, int mode);
    bool removeItem(int index);

signals:
    void countChanged();

private:
    struct Item {
        QString name;
        QString payload;
        int mode = 0;
    };

    QVector<Item> m_items;
};

