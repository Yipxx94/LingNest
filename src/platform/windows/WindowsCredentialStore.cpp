#include "platform/windows/WindowsCredentialStore.h"

#include <QByteArray>

#ifdef Q_OS_WIN
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <wincred.h>
#endif

namespace lingnest::platform::windows {
namespace {

void setError(QString* errorMessage, const QString& message)
{
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

#ifdef Q_OS_WIN
QString credentialError(const QString& operation, unsigned long errorCode)
{
    return QStringLiteral("%1失败（Windows 错误 %2）。")
        .arg(operation)
        .arg(errorCode);
}
#endif

} // namespace

std::optional<QString> WindowsCredentialStore::readSecret(
    const QString& target,
    QString* errorMessage)
{
#ifdef Q_OS_WIN
    PCREDENTIALW credential = nullptr;
    const auto* targetName = reinterpret_cast<const wchar_t*>(target.utf16());
    if (!CredReadW(targetName, CRED_TYPE_GENERIC, 0, &credential)) {
        const DWORD errorCode = GetLastError();
        if (errorCode == ERROR_NOT_FOUND) {
            return std::nullopt;
        }
        setError(errorMessage, credentialError(QStringLiteral("读取安全凭据"), errorCode));
        return std::nullopt;
    }

    QByteArray bytes(
        reinterpret_cast<const char*>(credential->CredentialBlob),
        static_cast<int>(credential->CredentialBlobSize));
    const QString secret = QString::fromUtf8(bytes);
    if (!bytes.isEmpty()) {
        SecureZeroMemory(bytes.data(), static_cast<size_t>(bytes.size()));
    }
    CredFree(credential);
    return secret;
#else
    Q_UNUSED(target)
    setError(errorMessage, QStringLiteral("当前平台不支持 Windows 凭据管理器。"));
    return std::nullopt;
#endif
}

bool WindowsCredentialStore::writeSecret(
    const QString& target,
    const QString& secret,
    QString* errorMessage)
{
#ifdef Q_OS_WIN
    QByteArray bytes = secret.toUtf8();
    if (bytes.isEmpty()) {
        setError(errorMessage, QStringLiteral("API Key 不能为空。"));
        return false;
    }
    if (bytes.size() > CRED_MAX_CREDENTIAL_BLOB_SIZE) {
        SecureZeroMemory(bytes.data(), static_cast<size_t>(bytes.size()));
        setError(errorMessage, QStringLiteral("API Key 超过 Windows 凭据长度限制。"));
        return false;
    }

    const QString userName = QStringLiteral("LingNest");
    CREDENTIALW credential {};
    credential.Type = CRED_TYPE_GENERIC;
    credential.TargetName = const_cast<wchar_t*>(
        reinterpret_cast<const wchar_t*>(target.utf16()));
    credential.CredentialBlobSize = static_cast<DWORD>(bytes.size());
    credential.CredentialBlob = reinterpret_cast<LPBYTE>(bytes.data());
    credential.Persist = CRED_PERSIST_LOCAL_MACHINE;
    credential.UserName = const_cast<wchar_t*>(
        reinterpret_cast<const wchar_t*>(userName.utf16()));

    const BOOL succeeded = CredWriteW(&credential, 0);
    const DWORD errorCode = succeeded ? ERROR_SUCCESS : GetLastError();
    SecureZeroMemory(bytes.data(), static_cast<size_t>(bytes.size()));
    if (!succeeded) {
        setError(errorMessage, credentialError(QStringLiteral("保存安全凭据"), errorCode));
        return false;
    }
    return true;
#else
    Q_UNUSED(target)
    Q_UNUSED(secret)
    setError(errorMessage, QStringLiteral("当前平台不支持 Windows 凭据管理器。"));
    return false;
#endif
}

bool WindowsCredentialStore::removeSecret(
    const QString& target,
    QString* errorMessage)
{
#ifdef Q_OS_WIN
    const auto* targetName = reinterpret_cast<const wchar_t*>(target.utf16());
    if (CredDeleteW(targetName, CRED_TYPE_GENERIC, 0)) {
        return true;
    }

    const DWORD errorCode = GetLastError();
    if (errorCode == ERROR_NOT_FOUND) {
        return true;
    }
    setError(errorMessage, credentialError(QStringLiteral("删除安全凭据"), errorCode));
    return false;
#else
    Q_UNUSED(target)
    setError(errorMessage, QStringLiteral("当前平台不支持 Windows 凭据管理器。"));
    return false;
#endif
}

} // namespace lingnest::platform::windows
