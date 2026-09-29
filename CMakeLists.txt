cmake_minimum_required(VERSION 3.21)

project(
    LingNest
    VERSION 0.1.3
    DESCRIPTION "A multi-character AI desktop pet framework"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# Qt 6 is the product target. Qt 5 remains a temporary bootstrap fallback so
# the skeleton can be compiled on machines that have not installed Qt 6 yet.
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2 Sql Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2 Sql Widgets)

if(QT_VERSION_MAJOR LESS 6)
    message(WARNING
        "Qt ${QT_VERSION} bootstrap fallback selected. "
        "Install Qt 6 and configure with its prefix for product development."
    )
endif()

add_library(lingnest_runtime STATIC
    src/app/SingleInstanceGuard.cpp
    src/app/SingleInstanceGuard.h
    src/ai/AIChatTypes.h
    src/ai/IAIProvider.cpp
    src/ai/IAIProvider.h
    src/ai/OpenAICompatibleProvider.cpp
    src/ai/OpenAICompatibleProvider.h
    src/animation/AnimationCatalog.cpp
    src/animation/AnimationCatalog.h
    src/animation/IAnimationPlayer.cpp
    src/animation/IAnimationPlayer.h
    src/animation/PngSequenceAnimationPlayer.cpp
    src/animation/PngSequenceAnimationPlayer.h
    src/character/CharacterDefinition.h
    src/character/CharacterLoader.cpp
    src/character/CharacterLoader.h
    src/config/ConfigManager.cpp
    src/config/ConfigManager.h
    src/config/ISecretStore.h
    src/interaction/InteractionController.cpp
    src/interaction/InteractionController.h
    src/memory/ExplicitMemoryParser.cpp
    src/memory/ExplicitMemoryParser.h
    src/memory/MemoryRepository.cpp
    src/memory/MemoryRepository.h
    src/memory/MemoryTypes.h
    src/memory/PromptBuilder.cpp
    src/memory/PromptBuilder.h
    src/pet/PetScheduler.cpp
    src/pet/PetScheduler.h
    src/pet/PetState.h
    src/pet/PetStateMachine.cpp
    src/pet/PetStateMachine.h
    src/platform/windows/WindowsCredentialStore.cpp
    src/platform/windows/WindowsCredentialStore.h
    src/ui/AISettingsController.cpp
    src/ui/AISettingsController.h
    src/ui/ChatController.cpp
    src/ui/ChatController.h
    src/ui/ChatMessageModel.cpp
    src/ui/ChatMessageModel.h
    src/ui/PetWindowController.cpp
    src/ui/PetWindowController.h
    src/ui/SpeechBubbleController.cpp
    src/ui/SpeechBubbleController.h
    src/ui/SystemTrayController.cpp
    src/ui/SystemTrayController.h
)

target_include_directories(lingnest_runtime PUBLIC src)

target_link_libraries(lingnest_runtime PUBLIC
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::Sql
    Qt${QT_VERSION_MAJOR}::Widgets
)

if(WIN32)
    target_link_libraries(lingnest_runtime PUBLIC Advapi32)
endif()

add_executable(LingNest WIN32
    resources.qrc
    src/main.cpp
    src/app/Application.cpp
    src/app/Application.h
)

if(WIN32)
    enable_language(RC)
    target_sources(LingNest PRIVATE resources/LingNest.rc)
    set_source_files_properties(resources/LingNest.rc PROPERTIES
        OBJECT_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/lingnest.ico"
    )
endif()

target_include_directories(LingNest PRIVATE src)

target_compile_definitions(LingNest PRIVATE
    LINGNEST_VERSION="${PROJECT_VERSION}"
    LINGNEST_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    LINGNEST_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    LINGNEST_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)

target_link_libraries(LingNest PRIVATE
    lingnest_runtime
    Qt${QT_VERSION_MAJOR}::Core
    Qt${QT_VERSION_MAJOR}::Gui
    Qt${QT_VERSION_MAJOR}::Qml
    Qt${QT_VERSION_MAJOR}::Quick
    Qt${QT_VERSION_MAJOR}::QuickControls2
    Qt${QT_VERSION_MAJOR}::Widgets
)

function(lingnest_enable_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE
            /W4
            /permissive-
            /utf-8
            /Zc:__cplusplus
        )
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

lingnest_enable_warnings(lingnest_runtime)
lingnest_enable_warnings(LingNest)

file(GLOB_RECURSE LINGNEST_CHARACTER_RUNTIME_FILES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/characters/*/character.json"
    "${CMAKE_CURRENT_SOURCE_DIR}/characters/*/prompt.md"
    "${CMAKE_CURRENT_SOURCE_DIR}/characters/*/assets/*"
)

set_property(TARGET LingNest APPEND PROPERTY
    LINK_DEPENDS "${LINGNEST_CHARACTER_RUNTIME_FILES}"
)

add_custom_command(TARGET LingNest POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        -DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}/characters
        -DDESTINATION_DIR=$<TARGET_FILE_DIR:LingNest>/characters
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/StageCharacters.cmake
    COMMENT "Staging runtime character packages"
    VERBATIM
)

include(CTest)
if(BUILD_TESTING)
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Test)
    add_subdirectory(tests)

    add_test(NAME lingnest_startup_smoke
        COMMAND $<TARGET_FILE:LingNest> --smoke-test)
    set_tests_properties(lingnest_startup_smoke PROPERTIES
        TIMEOUT 10
        ENVIRONMENT "QT_QUICK_BACKEND=software"
    )
endif()

install(TARGETS LingNest RUNTIME DESTINATION .)

if(WIN32)
    get_target_property(_lingnest_qmake_executable
        Qt${QT_VERSION_MAJOR}::qmake IMPORTED_LOCATION)
    get_filename_component(_lingnest_qt_bin_dir
        "${_lingnest_qmake_executable}" DIRECTORY)
    find_program(LINGNEST_WINDEPLOYQT_EXECUTABLE
        NAMES windeployqt
        HINTS "${_lingnest_qt_bin_dir}"
        REQUIRED)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LINGNEST_PACKAGE_ARCH "x64")
    else()
        set(LINGNEST_PACKAGE_ARCH "x86")
    endif()
    set(LINGNEST_PACKAGE_BASENAME
        "LingNest-v${PROJECT_VERSION}-windows-${LINGNEST_PACKAGE_ARCH}-portable")

    get_filename_component(_lingnest_vc_root "${CMAKE_CXX_COMPILER}" DIRECTORY)
    foreach(_lingnest_parent_step RANGE 1 6)
        get_filename_component(_lingnest_vc_root "${_lingnest_vc_root}" DIRECTORY)
    endforeach()
    file(GLOB _lingnest_vc_runtime_candidates LIST_DIRECTORIES true
        "${_lingnest_vc_root}/Redist/MSVC/*/x64/Microsoft.VC*.CRT")
    list(SORT _lingnest_vc_runtime_candidates
        COMPARE NATURAL ORDER DESCENDING)
    if(_lingnest_vc_runtime_candidates)
        list(GET _lingnest_vc_runtime_candidates 0 LINGNEST_VC_RUNTIME_DIR)
    else()
        set(LINGNEST_VC_RUNTIME_DIR "")
    endif()

    add_custom_target(package_windows
        COMMAND ${CMAKE_COMMAND}
            "-DSOURCE_EXE=$<TARGET_FILE:LingNest>"
            "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
            "-DOUTPUT_ROOT=${CMAKE_BINARY_DIR}/package"
            "-DPACKAGE_BASENAME=${LINGNEST_PACKAGE_BASENAME}"
            "-DPROJECT_VERSION=${PROJECT_VERSION}"
            "-DQT_VERSION=${QT_VERSION}"
            "-DQT_MAJOR_VERSION=${QT_VERSION_MAJOR}"
            "-DWINDEPLOYQT_EXECUTABLE=${LINGNEST_WINDEPLOYQT_EXECUTABLE}"
            "-DVC_RUNTIME_DIR=${LINGNEST_VC_RUNTIME_DIR}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeployWindows.cmake"
        DEPENDS LingNest
        COMMENT "Creating LingNest ${PROJECT_VERSION} Windows portable package"
        VERBATIM
    )

    set(LINGNEST_INSTALLER_BASENAME
        "LingNest-v${PROJECT_VERSION}-windows-${LINGNEST_PACKAGE_ARCH}-setup")
    find_program(LINGNEST_ISCC_EXECUTABLE
        NAMES ISCC iscc
        HINTS
            "$ENV{LOCALAPPDATA}/Programs/Inno Setup 6"
            "C:/Program Files (x86)/Inno Setup 6"
            "C:/Program Files/Inno Setup 6")

    if(LINGNEST_ISCC_EXECUTABLE)
        file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/installer")
        configure_file(
            "${CMAKE_CURRENT_SOURCE_DIR}/installer/LingNest.iss.in"
            "${CMAKE_BINARY_DIR}/installer/LingNest.iss"
            @ONLY)

        add_custom_target(package_installer
            COMMAND "${LINGNEST_ISCC_EXECUTABLE}"
                "/Qp"
                "${CMAKE_BINARY_DIR}/installer/LingNest.iss"
            COMMAND ${CMAKE_COMMAND}
                "-DSOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
                "-DPACKAGE_ROOT=${CMAKE_BINARY_DIR}/package"
                "-DPORTABLE_BASENAME=${LINGNEST_PACKAGE_BASENAME}"
                "-DINSTALLER_BASENAME=${LINGNEST_INSTALLER_BASENAME}"
                "-DPROJECT_VERSION=${PROJECT_VERSION}"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/PublishWindowsRelease.cmake"
            DEPENDS package_windows
            COMMENT "Creating LingNest ${PROJECT_VERSION} Windows installer and dist release"
            VERBATIM
        )
    else()
        message(WARNING
            "Inno Setup 6 was not found. The package_installer target will not be available. "
            "Install Inno Setup 6 or set LINGNEST_ISCC_EXECUTABLE explicitly.")
    endif()
endif()
