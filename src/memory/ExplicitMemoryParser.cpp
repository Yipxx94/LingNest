#include "memory/ExplicitMemoryParser.h"

#include <QRegularExpression>

namespace lingnest::memory {
namespace {

QString withoutTrailingPunctuation(QString value)
{
    value = value.trimmed();
    value.remove(QRegularExpression(QStringLiteral("[。！？!?，,；;：:]+$")));
    return value.trimmed();
}

QString normalizedMemoryKey(const QString& value)
{
    QString normalized = value.toCaseFolded().simplified();
    normalized.remove(QRegularExpression(
        QStringLiteral("[\\p{P}\\p{S}\\s]+"),
        QRegularExpression::UseUnicodePropertiesOption));
    return normalized.left(160);
}

bool captureProfileValue(
    const QString& content,
    const QString& pattern,
    QString* value)
{
    const QRegularExpression expression(
        pattern, QRegularExpression::UseUnicodePropertiesOption);
    const QRegularExpressionMatch match = expression.match(content);
    if (!match.hasMatch()) {
        return false;
    }

    *value = withoutTrailingPunctuation(match.captured(1));
    return !value->isEmpty();
}

} // namespace

std::optional<ExplicitMemory> ExplicitMemoryParser::parse(
    const QString& userMessage)
{
    const QRegularExpression commandExpression(
        QStringLiteral("^(?:请)?记住(?:一下)?[\\s，,：:。]*(.+)$"),
        QRegularExpression::UseUnicodePropertiesOption);
    const QRegularExpressionMatch commandMatch =
        commandExpression.match(userMessage.trimmed());
    if (!commandMatch.hasMatch()) {
        return std::nullopt;
    }

    const QString content = withoutTrailingPunctuation(commandMatch.captured(1));
    if (content.isEmpty()) {
        return std::nullopt;
    }

    ExplicitMemory result;
    result.content = content;

    QString profileValue;
    if (captureProfileValue(
            content,
            QStringLiteral("^(?:我叫|我的名字(?:是|叫))\\s*(.+)$"),
            &profileValue)) {
        result.type = QStringLiteral("Fact");
        result.key = QStringLiteral("profile:name");
        result.profileKey = QStringLiteral("name");
        result.profileValue = profileValue;
        return result;
    }
    if (captureProfileValue(
            content,
            QStringLiteral("^我的生日(?:是|在)\\s*(.+)$"),
            &profileValue)) {
        result.type = QStringLiteral("Fact");
        result.key = QStringLiteral("profile:birthday");
        result.profileKey = QStringLiteral("birthday");
        result.profileValue = profileValue;
        return result;
    }
    if (captureProfileValue(
            content,
            QStringLiteral("^(?:我住在|我的所在地是)\\s*(.+)$"),
            &profileValue)) {
        result.type = QStringLiteral("Fact");
        result.key = QStringLiteral("profile:location");
        result.profileKey = QStringLiteral("location");
        result.profileValue = profileValue;
        return result;
    }

    if (content.contains(QStringLiteral("喜欢"))
        || content.contains(QStringLiteral("偏好"))) {
        result.type = QStringLiteral("Preference");
    } else if (content.contains(QStringLiteral("朋友"))
        || content.contains(QStringLiteral("家人"))
        || content.contains(QStringLiteral("同事"))
        || content.contains(QStringLiteral("关系"))) {
        result.type = QStringLiteral("Relationship");
    } else if (content.startsWith(QStringLiteral("今天"))
        || content.startsWith(QStringLiteral("昨天"))
        || content.contains(QStringLiteral("发生了"))) {
        result.type = QStringLiteral("Event");
    } else {
        result.type = QStringLiteral("Fact");
    }

    result.key = normalizedMemoryKey(content);
    if (result.key.isEmpty()) {
        return std::nullopt;
    }
    return result;
}

} // namespace lingnest::memory
