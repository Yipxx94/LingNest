#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

namespace lingnest::ui {

class ChatMessageModel final : public QAbstractListModel {
    Q_OBJECT

public:
    enum Role {
        TextRole = Qt::UserRole + 1,
        SenderRole,
        IsUserRole
    };

    explicit ChatMessageModel(QObject* parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    [[nodiscard]] QVariant data(const QModelIndex& index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    void append(QString text, QString sender, bool isUser);
    void clear();

private:
    struct Message {
        QString text;
        QString sender;
        bool isUser {false};
    };

    QVector<Message> m_messages;
};

} // namespace lingnest::ui

