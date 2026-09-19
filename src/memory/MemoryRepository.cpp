#include "memory/MemoryRepository.h"

#include <utility>

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QStringList>
#include <QUuid>
#include <QVariant>
#include <QtGlobal>

namespace lingnest::memory {
namespace {

constexpr int currentSchemaVersion = 1;

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

QString currentTimestamp()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODateWithMs);
}

QString roleName(ai::ChatRole role)
{
    switch (role) {
    case ai::ChatRole::User:
        return QStringLiteral("user");
    case ai::ChatRole::Assistant:
        return QStringLiteral("assistant");
    case ai::ChatRole::System:
        return QStringLiteral("system");
    }
    return {};
}

ai::ChatRole roleFromName(const QString& role)
{
    return role == QStringLiteral("assistant")
        ? ai::ChatRole::Assistant
        : ai::ChatRole::User;
}

QDateTime timestampFromStorage(const QString& value)
{
    return QDateTime::fromString(value, Qt::ISODateWithMs);
}

QString queryError(const QString& operation, const QSqlQuery& query)
{
    return QStringLiteral("%1: %2")
        .arg(operation, query.lastError().text());
}

bool isCorruptionError(const QSqlError& error)
{
    const QString nativeCode = error.nativeErrorCode().trimmed();
    const QString text = error.text().toLower();
    return nativeCode == QStringLiteral("11")
        || nativeCode == QStringLiteral("26")
        || text.contains(QStringLiteral("malformed"))
        || text.contains(QStringLiteral("not a database"))
        || text.contains(QStringLiteral("database disk image is malformed"));
}

} // namespace

MemoryRepository::MemoryRepository(QString databasePath)
    : m_databasePath(std::move(databasePath))
    , m_connectionName(
          QStringLiteral("lingnest-memory-%1")
              .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

MemoryRepository::~MemoryRepository()
{
    close();
}

QString MemoryRepository::defaultDatabasePath()
{
    const QString dataDirectory =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    return QDir(dataDirectory).filePath(QStringLiteral("memory.sqlite3"));
}

const QString& MemoryRepository::databasePath() const noexcept
{
    return m_databasePath;
}

bool MemoryRepository::isOpen() const
{
    return m_database.isValid() && m_database.isOpen();
}

bool MemoryRepository::open(QString* errorMessage)
{
    m_recoverableOpenFailure = false;
    if (isOpen()) {
        return true;
    }

    if (m_databasePath != QStringLiteral(":memory:")) {
        QDir parentDirectory = QFileInfo(m_databasePath).dir();
        if (!parentDirectory.exists()
            && !parentDirectory.mkpath(QStringLiteral("."))) {
            setError(
                errorMessage,
                QStringLiteral("Cannot create memory database directory: %1")
                    .arg(QDir::toNativeSeparators(parentDirectory.absolutePath())));
            return false;
        }
    }

    m_database = QSqlDatabase::addDatabase(
        QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(m_databasePath);
    if (!m_database.open()) {
        m_recoverableOpenFailure = isCorruptionError(m_database.lastError());
        const QString message = QStringLiteral("Cannot open memory database %1: %2")
            .arg(
                QDir::toNativeSeparators(m_databasePath),
                m_database.lastError().text());
        close();
        setError(errorMessage, message);
        return false;
    }

    QSqlQuery configurationQuery(m_database);
    const QStringList pragmas {
        QStringLiteral("PRAGMA foreign_keys = ON"),
        QStringLiteral("PRAGMA busy_timeout = 3000"),
        QStringLiteral("PRAGMA journal_mode = WAL")
    };
    for (const QString& pragma : pragmas) {
        if (!configurationQuery.exec(pragma)) {
            m_recoverableOpenFailure = isCorruptionError(
                configurationQuery.lastError());
            const QString message = queryError(
                QStringLiteral("Cannot configure memory database"),
                configurationQuery);
            close();
            setError(errorMessage, message);
            return false;
        }
        configurationQuery.finish();
    }

    if (!checkIntegrity(errorMessage)) {
        close();
        return false;
    }

    if (!migrate(errorMessage)) {
        close();
        return false;
    }
    return true;
}

bool MemoryRepository::openWithRecovery(QString* recoveryMessage)
{
    if (recoveryMessage != nullptr) {
        recoveryMessage->clear();
    }

    QString openError;
    if (open(&openError)) {
        return true;
    }

    if (!m_recoverableOpenFailure
        || m_databasePath == QStringLiteral(":memory:")
        || !QFileInfo::exists(m_databasePath)) {
        setError(recoveryMessage, openError);
        return false;
    }

    close();
    const QString timestamp = QDateTime::currentDateTimeUtc().toString(
        QStringLiteral("yyyyMMdd-HHmmss-zzz"));
    const QString backupPath = QStringLiteral("%1.corrupt-%2")
                                   .arg(m_databasePath, timestamp);
    if (!QFile::rename(m_databasePath, backupPath)) {
        setError(
            recoveryMessage,
            QStringLiteral("%1 The damaged database could not be backed up.")
                .arg(openError));
        return false;
    }

    for (const QString& suffix : {QStringLiteral("-wal"), QStringLiteral("-shm")}) {
        const QString sidecarPath = m_databasePath + suffix;
        if (QFileInfo::exists(sidecarPath)) {
            QFile::rename(sidecarPath, backupPath + suffix);
        }
    }

    QString retryError;
    if (!open(&retryError)) {
        setError(
            recoveryMessage,
            QStringLiteral(
                "%1 The damaged database was preserved as %2, but a clean "
                "database could not be created: %3")
                .arg(
                    openError,
                    QDir::toNativeSeparators(backupPath),
                    retryError));
        return false;
    }

    setError(
        recoveryMessage,
        QStringLiteral(
            "The damaged memory database was reset. A backup was preserved as %1.")
            .arg(QDir::toNativeSeparators(backupPath)));
    return true;
}

void MemoryRepository::close()
{
    if (m_database.isValid()) {
        m_database.close();
        m_database = QSqlDatabase();
    }
    if (QSqlDatabase::contains(m_connectionName)) {
        QSqlDatabase::removeDatabase(m_connectionName);
    }
}

bool MemoryRepository::checkIntegrity(QString* errorMessage)
{
    QSqlQuery query(m_database);
    if (!query.exec(QStringLiteral("PRAGMA quick_check"))) {
        m_recoverableOpenFailure = isCorruptionError(query.lastError());
        setError(
            errorMessage,
            queryError(QStringLiteral("Cannot verify memory database"), query));
        return false;
    }

    QStringList failures;
    while (query.next()) {
        const QString result = query.value(0).toString();
        if (result.compare(QStringLiteral("ok"), Qt::CaseInsensitive) != 0) {
            failures.append(result);
        }
    }
    if (failures.isEmpty()) {
        return true;
    }

    m_recoverableOpenFailure = true;
    setError(
        errorMessage,
        QStringLiteral("Memory database integrity check failed: %1")
            .arg(failures.join(QStringLiteral("; "))));
    return false;
}

bool MemoryRepository::appendConversation(
    const QString& characterId,
    ai::ChatRole role,
    const QString& content,
    QString* errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }
    if (role == ai::ChatRole::System) {
        setError(errorMessage, QStringLiteral("System messages are not persisted as conversation."));
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO conversation(character_id, role, content, created_at) "
        "VALUES(?, ?, ?, ?)"));
    query.addBindValue(characterId);
    query.addBindValue(roleName(role));
    query.addBindValue(content);
    query.addBindValue(currentTimestamp());
    if (!query.exec()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot append conversation"), query));
        return false;
    }
    return true;
}

QVector<ConversationEntry> MemoryRepository::recentConversation(
    const QString& characterId,
    int limit,
    QString* errorMessage) const
{
    QVector<ConversationEntry> entries;
    if (!ensureOpen(errorMessage)) {
        return entries;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, role, content, created_at FROM ("
        "  SELECT id, role, content, created_at FROM conversation "
        "  WHERE character_id = ? ORDER BY id DESC LIMIT ?"
        ") recent ORDER BY id ASC"));
    query.addBindValue(characterId);
    query.addBindValue(qBound(1, limit, 200));
    if (!query.exec()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot load recent conversation"), query));
        return entries;
    }

    while (query.next()) {
        ConversationEntry entry;
        entry.id = query.value(0).toLongLong();
        entry.role = roleFromName(query.value(1).toString());
        entry.content = query.value(2).toString();
        entry.createdAt = timestampFromStorage(query.value(3).toString());
        entries.append(std::move(entry));
    }
    return entries;
}

bool MemoryRepository::clearConversation(
    const QString& characterId,
    QString* errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "DELETE FROM conversation WHERE character_id = ?"));
    query.addBindValue(characterId);
    if (!query.exec()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot clear conversation"), query));
        return false;
    }
    return true;
}

bool MemoryRepository::upsertMemory(
    const QString& characterId,
    const QString& type,
    const QString& key,
    const QString& content,
    QString* errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    const QString timestamp = currentTimestamp();
    QSqlQuery updateQuery(m_database);
    updateQuery.prepare(QStringLiteral(
        "UPDATE memory SET content = ?, updated_at = ? "
        "WHERE character_id = ? AND memory_type = ? AND memory_key = ?"));
    updateQuery.addBindValue(content);
    updateQuery.addBindValue(timestamp);
    updateQuery.addBindValue(characterId);
    updateQuery.addBindValue(type);
    updateQuery.addBindValue(key);
    if (!updateQuery.exec()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot update memory"), updateQuery));
        return false;
    }
    if (updateQuery.numRowsAffected() > 0) {
        return true;
    }

    QSqlQuery insertQuery(m_database);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO memory("
        "character_id, memory_type, memory_key, content, created_at, updated_at"
        ") VALUES(?, ?, ?, ?, ?, ?)"));
    insertQuery.addBindValue(characterId);
    insertQuery.addBindValue(type);
    insertQuery.addBindValue(key);
    insertQuery.addBindValue(content);
    insertQuery.addBindValue(timestamp);
    insertQuery.addBindValue(timestamp);
    if (!insertQuery.exec()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot insert memory"), insertQuery));
        return false;
    }
    return true;
}

QVector<MemoryEntry> MemoryRepository::recentMemories(
    const QString& characterId,
    int limit,
    QString* errorMessage) const
{
    QVector<MemoryEntry> entries;
    if (!ensureOpen(errorMessage)) {
        return entries;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT id, memory_type, memory_key, content, created_at, updated_at "
        "FROM memory WHERE character_id = ? "
        "ORDER BY updated_at DESC, id DESC LIMIT ?"));
    query.addBindValue(characterId);
    query.addBindValue(qBound(1, limit, 200));
    if (!query.exec()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot load memories"), query));
        return entries;
    }

    while (query.next()) {
        MemoryEntry entry;
        entry.id = query.value(0).toLongLong();
        entry.type = query.value(1).toString();
        entry.key = query.value(2).toString();
        entry.content = query.value(3).toString();
        entry.createdAt = timestampFromStorage(query.value(4).toString());
        entry.updatedAt = timestampFromStorage(query.value(5).toString());
        entries.append(std::move(entry));
    }
    return entries;
}

bool MemoryRepository::upsertUserProfile(
    const QString& characterId,
    const QString& key,
    const QString& value,
    QString* errorMessage)
{
    if (!ensureOpen(errorMessage)) {
        return false;
    }

    const QString timestamp = currentTimestamp();
    QSqlQuery updateQuery(m_database);
    updateQuery.prepare(QStringLiteral(
        "UPDATE user_profile SET profile_value = ?, updated_at = ? "
        "WHERE character_id = ? AND profile_key = ?"));
    updateQuery.addBindValue(value);
    updateQuery.addBindValue(timestamp);
    updateQuery.addBindValue(characterId);
    updateQuery.addBindValue(key);
    if (!updateQuery.exec()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot update user profile"), updateQuery));
        return false;
    }
    if (updateQuery.numRowsAffected() > 0) {
        return true;
    }

    QSqlQuery insertQuery(m_database);
    insertQuery.prepare(QStringLiteral(
        "INSERT INTO user_profile("
        "character_id, profile_key, profile_value, updated_at"
        ") VALUES(?, ?, ?, ?)"));
    insertQuery.addBindValue(characterId);
    insertQuery.addBindValue(key);
    insertQuery.addBindValue(value);
    insertQuery.addBindValue(timestamp);
    if (!insertQuery.exec()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot insert user profile"), insertQuery));
        return false;
    }
    return true;
}

QHash<QString, QString> MemoryRepository::userProfile(
    const QString& characterId,
    QString* errorMessage) const
{
    QHash<QString, QString> profile;
    if (!ensureOpen(errorMessage)) {
        return profile;
    }

    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "SELECT profile_key, profile_value FROM user_profile "
        "WHERE character_id = ? ORDER BY profile_key ASC"));
    query.addBindValue(characterId);
    if (!query.exec()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot load user profile"), query));
        return profile;
    }

    while (query.next()) {
        profile.insert(query.value(0).toString(), query.value(1).toString());
    }
    return profile;
}

bool MemoryRepository::migrate(QString* errorMessage)
{
    QSqlQuery versionQuery(m_database);
    if (!versionQuery.exec(QStringLiteral("PRAGMA user_version"))
        || !versionQuery.next()) {
        setError(errorMessage, queryError(
            QStringLiteral("Cannot read memory schema version"), versionQuery));
        return false;
    }

    const int schemaVersion = versionQuery.value(0).toInt();
    versionQuery.finish();
    if (schemaVersion > currentSchemaVersion) {
        setError(
            errorMessage,
            QStringLiteral("Memory database schema %1 is newer than supported version %2.")
                .arg(schemaVersion)
                .arg(currentSchemaVersion));
        return false;
    }
    if (schemaVersion == currentSchemaVersion) {
        return true;
    }

    if (!m_database.transaction()) {
        setError(errorMessage, QStringLiteral("Cannot begin memory database migration: %1")
            .arg(m_database.lastError().text()));
        return false;
    }

    const QStringList statements {
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS conversation("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "character_id TEXT NOT NULL, "
            "role TEXT NOT NULL CHECK(role IN ('user', 'assistant')), "
            "content TEXT NOT NULL, "
            "created_at TEXT NOT NULL)"),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_conversation_character_id "
            "ON conversation(character_id, id DESC)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS memory("
            "id INTEGER PRIMARY KEY AUTOINCREMENT, "
            "character_id TEXT NOT NULL, "
            "memory_type TEXT NOT NULL, "
            "memory_key TEXT NOT NULL, "
            "content TEXT NOT NULL, "
            "created_at TEXT NOT NULL, "
            "updated_at TEXT NOT NULL, "
            "UNIQUE(character_id, memory_type, memory_key))"),
        QStringLiteral(
            "CREATE INDEX IF NOT EXISTS idx_memory_character_updated "
            "ON memory(character_id, updated_at DESC)"),
        QStringLiteral(
            "CREATE TABLE IF NOT EXISTS user_profile("
            "character_id TEXT NOT NULL, "
            "profile_key TEXT NOT NULL, "
            "profile_value TEXT NOT NULL, "
            "updated_at TEXT NOT NULL, "
            "PRIMARY KEY(character_id, profile_key))"),
        QStringLiteral("PRAGMA user_version = 1")
    };

    QSqlQuery migrationQuery(m_database);
    for (const QString& statement : statements) {
        if (!migrationQuery.exec(statement)) {
            const QString message = queryError(
                QStringLiteral("Cannot migrate memory database"), migrationQuery);
            m_database.rollback();
            setError(errorMessage, message);
            return false;
        }
        migrationQuery.finish();
    }

    if (!m_database.commit()) {
        setError(errorMessage, QStringLiteral("Cannot commit memory database migration: %1")
            .arg(m_database.lastError().text()));
        return false;
    }
    return true;
}

bool MemoryRepository::ensureOpen(QString* errorMessage) const
{
    if (isOpen()) {
        return true;
    }
    setError(errorMessage, QStringLiteral("Memory database is not open."));
    return false;
}

} // namespace lingnest::memory
