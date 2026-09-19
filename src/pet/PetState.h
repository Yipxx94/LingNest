#pragma once

#include <QMetaType>
#include <QObject>

namespace lingnest::pet {

Q_NAMESPACE

enum class PetState {
    Idle,
    Blink,
    Walk,
    Talk,
    Happy,
    Wave,
    Daze,
    Sleep,
    Thinking
};
Q_ENUM_NS(PetState)

enum class WalkDirection {
    Left,
    Right
};
Q_ENUM_NS(WalkDirection)

} // namespace lingnest::pet

Q_DECLARE_METATYPE(lingnest::pet::PetState)
Q_DECLARE_METATYPE(lingnest::pet::WalkDirection)
