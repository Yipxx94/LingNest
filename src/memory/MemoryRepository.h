#pragma once

#include <QHash>
#include <QSqlDatabase>
#include <QString>
#include <QVector>

#include "ai/AIChatTypes.h"
#include "memory/MemoryTypes.h"

namespace lingnest::memory {

class MemoryRepository final {
public:
    explicit MemoryRepository(QString databasePath = defaultDatabasePath());
    ~MemoryRepository();

    MemoryRepository(const MemoryRepository&) = delete;
    MemoryRepository& operator=(const MemoryRepository&) = delete;

    [[nodiscard]] static QString defaultDatabasePath();
    [[nodiscard]] const QString& databasePath() const noexcept;
    [[nodiscard]] bool isOpen() const;

    bool open(QString* errorMessage = nullptr);
    bool openWithRecovery(QString* recoveryMessage = nullptr);
    void close();

    bool appendConversation(
        const QString& characterId,
        ai::ChatRole role,
        const QString& content,
        QString* errorMessage = nullptr);
    [[nodiscard]] QVector<ConversationEntry> recentConversation(
        const QString& characterId,
        int limit,
        QString* errorMessage = nullptr) const;
    bool clearConversation(
        const QString& characterId,
        QString* errorMessage = nullptr);

    bool upsertMemory(
        const QString& characterId,
        const QString& type,
        const QString& key,
        const QString& content,
        QString* errorMessage = nullptr);
    [[nodiscard]] QVector<MemoryEntry> recentMemories(
        const QString& characterId,
        int limit,
        QString* errorMessage = nullptr) const;

    bool upsertUserProfile(
        const QString& characterId,
        const QString& key,
        const QString& value,
        QString* errorMessage = nullptr);
    [[nodiscard]] QHash<QString, QString> userProfile(
        const QString& characterId,
        QString* errorMessage = nullptr) const;

private:
    bool checkIntegrity(QString* errorMessage);
    bool migrate(QString* errorMessage);
    [[nodiscard]] bool ensureOpen(QString* errorMessage) const;

    QString m_databasePath;
    QString m_connectionName;
    QSqlDatabase m_database;
    bool m_recoverableOpenFailure {false};
};

} // namespace lingnest::memory
