#pragma once

#include <QHash>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QtGlobal>

namespace lingnest::character {

struct CharacterDefinition {
    QString id;
    QString name;
    QString displayName;
    QString description;
    QStringList personality;
    QStringList availableAnimations;
    QHash<QString, QString> stateAnimations;
    QString packageDirectory;
    QString promptFilePath;
    QString animationManifestPath;
    QString defaultAnimation;
    QUrl avatarUrl;
    QUrl defaultFrameUrl;
    qreal scale {1.0};
};

} // namespace lingnest::character
