#include "ui/ChatMessageModel.h"

#include <utility>

namespace lingnest::ui {

ChatMessageModel::ChatMessageModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

int ChatMessageModel::rowCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : m_messages.size();
}

QVariant ChatMessageModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_messages.size()) {
        return {};
    }

    const Message& message = m_messages.at(index.row());
    switch (role) {
    case TextRole:
        return message.text;
    case SenderRole:
        return message.sender;
    case IsUserRole:
        return message.isUser;
    default:
        return {};
    }
}

QHash<int, QByteArray> ChatMessageModel::roleNames() const
{
    return {
        {TextRole, QByteArrayLiteral("messageText")},
        {SenderRole, QByteArrayLiteral("messageSender")},
        {IsUserRole, QByteArrayLiteral("messageIsUser")}
    };
}

void ChatMessageModel::append(QString text, QString sender, bool isUser)
{
    const int row = m_messages.size();
    beginInsertRows(QModelIndex(), row, row);
    m_messages.append({std::move(text), std::move(sender), isUser});
    endInsertRows();
}

void ChatMessageModel::clear()
{
    if (m_messages.isEmpty()) {
        return;
    }

    beginResetModel();
    m_messages.clear();
    endResetModel();
}

} // namespace lingnest::ui

