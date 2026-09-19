cmake_minimum_required(VERSION 3.21)

foreach(required_variable IN ITEMS
        SOURCE_EXE
        SOURCE_DIR
        OUTPUT_ROOT
        PACKAGE_BASENAME
        PROJECT_VERSION
        QT_VERSION
        QT_MAJOR_VERSION
        WINDEPLOYQT_EXECUTABLE
        VC_RUNTIME_DIR)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} must be provided")
    endif()
endforeach()

if(NOT EXISTS "${SOURCE_EXE}")
    message(FATAL_ERROR "Release executable does not exist: ${SOURCE_EXE}")
endif()
if(NOT EXISTS "${WINDEPLOYQT_EXECUTABLE}")
    message(FATAL_ERROR "windeployqt does not exist: ${WINDEPLOYQT_EXECUTABLE}")
endif()
if(NOT IS_DIRECTORY "${VC_RUNTIME_DIR}")
    message(FATAL_ERROR "Visual C++ app-local runtime was not found: ${VC_RUNTIME_DIR}")
endif()

set(staging_directory "${OUTPUT_ROOT}/${PACKAGE_BASENAME}")
set(archive_path "${OUTPUT_ROOT}/${PACKAGE_BASENAME}.zip")
set(checksum_path "${OUTPUT_ROOT}/${PACKAGE_BASENAME}.sha256")
set(manifest_path "${OUTPUT_ROOT}/${PACKAGE_BASENAME}.manifest.json")

# These paths are fully controlled by the build tree and package base name.
file(REMOVE_RECURSE "${staging_directory}")
file(REMOVE "${archive_path}" "${checksum_path}" "${manifest_path}")
file(MAKE_DIRECTORY "${staging_directory}" "${staging_directory}/docs")

file(COPY "${SOURCE_EXE}" DESTINATION "${staging_directory}")
file(COPY "${SOURCE_DIR}/README.md" DESTINATION "${staging_directory}")

foreach(document IN ITEMS
        user-guide.md
        privacy.md
        third-party-notices.md
        "release-notes-v${PROJECT_VERSION}.md"
        "release-validation-v${PROJECT_VERSION}.md")
    if(NOT EXISTS "${SOURCE_DIR}/docs/${document}")
        message(FATAL_ERROR "Required release document is missing: docs/${document}")
    endif()
    file(COPY "${SOURCE_DIR}/docs/${document}"
        DESTINATION "${staging_directory}/docs")
endforeach()

execute_process(
    COMMAND "${CMAKE_COMMAND}"
        "-DSOURCE_DIR=${SOURCE_DIR}/characters"
        "-DDESTINATION_DIR=${staging_directory}/characters"
        -P "${SOURCE_DIR}/cmake/StageCharacters.cmake"
    RESULT_VARIABLE character_stage_result
    OUTPUT_VARIABLE character_stage_output
    ERROR_VARIABLE character_stage_error
)
if(NOT character_stage_result EQUAL 0)
    message(FATAL_ERROR
        "Character staging failed (${character_stage_result}).\n"
        "${character_stage_output}\n${character_stage_error}")
endif()

execute_process(
    COMMAND "${WINDEPLOYQT_EXECUTABLE}"
        --release
        --compiler-runtime
        --no-translations
        --qmldir "${SOURCE_DIR}/qml"
        "${staging_directory}/LingNest.exe"
    RESULT_VARIABLE deploy_result
    OUTPUT_VARIABLE deploy_output
    ERROR_VARIABLE deploy_error
)
if(NOT deploy_result EQUAL 0)
    message(FATAL_ERROR
        "windeployqt failed (${deploy_result}).\n${deploy_output}\n${deploy_error}")
endif()

# windeployqt places the redistributable installer beside the application.
# A portable build instead carries the supported app-local CRT DLL set.
file(REMOVE "${staging_directory}/vc_redist.x64.exe")
file(GLOB vc_runtime_files LIST_DIRECTORIES false "${VC_RUNTIME_DIR}/*.dll")
if(NOT vc_runtime_files)
    message(FATAL_ERROR "No Visual C++ runtime DLLs were found in ${VC_RUNTIME_DIR}")
endif()
file(COPY ${vc_runtime_files} DESTINATION "${staging_directory}")

set(required_package_files
    "LingNest.exe"
    "characters/yuai/character.json"
    "characters/yuai/prompt.md"
    "characters/yuai/assets/manifest.json"
    "platforms/qwindows.dll"
    "Qt${QT_MAJOR_VERSION}Core.dll"
    "Qt${QT_MAJOR_VERSION}Gui.dll"
    "Qt${QT_MAJOR_VERSION}Qml.dll"
    "Qt${QT_MAJOR_VERSION}Quick.dll"
    "msvcp140.dll"
    "vcruntime140.dll"
    "vcruntime140_1.dll"
)
foreach(required_file IN LISTS required_package_files)
    if(NOT EXISTS "${staging_directory}/${required_file}")
        message(FATAL_ERROR "Deployed package is missing ${required_file}")
    endif()
endforeach()

string(TIMESTAMP build_timestamp "%Y-%m-%dT%H:%M:%SZ" UTC)
file(WRITE "${staging_directory}/VERSION.txt"
    "LingNest ${PROJECT_VERSION}\n"
    "Platform: Windows x64\n"
    "Qt runtime: ${QT_VERSION}\n"
    "Visual C++ runtime: app-local\n"
    "Built: ${build_timestamp}\n")

file(GLOB_RECURSE packaged_files
    LIST_DIRECTORIES false
    RELATIVE "${staging_directory}"
    "${staging_directory}/*")
list(SORT packaged_files)
list(LENGTH packaged_files packaged_file_count)

set(forbidden_files)
foreach(relative_path IN LISTS packaged_files)
    get_filename_component(file_name "${relative_path}" NAME)
    string(TOLOWER "${file_name}" lower_file_name)
    if(lower_file_name STREQUAL "apikey.txt"
        OR lower_file_name STREQUAL "config.json"
        OR lower_file_name STREQUAL "memory.sqlite3"
        OR lower_file_name MATCHES "\\.(log|pdb|ilk|exp|lib|key|pem)$")
        list(APPEND forbidden_files "${relative_path}")
    endif()
endforeach()
if(forbidden_files)
    list(JOIN forbidden_files "\n  " forbidden_file_list)
    message(FATAL_ERROR
        "Sensitive or development-only files entered the package:\n  "
        "${forbidden_file_list}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E tar cf "${archive_path}"
        --format=zip "${PACKAGE_BASENAME}"
    WORKING_DIRECTORY "${OUTPUT_ROOT}"
    RESULT_VARIABLE archive_result
    OUTPUT_VARIABLE archive_output
    ERROR_VARIABLE archive_error
)
if(NOT archive_result EQUAL 0 OR NOT EXISTS "${archive_path}")
    message(FATAL_ERROR
        "ZIP creation failed (${archive_result}).\n${archive_output}\n${archive_error}")
endif()

file(SHA256 "${archive_path}" archive_sha256)
file(SIZE "${archive_path}" archive_size)
get_filename_component(archive_name "${archive_path}" NAME)
file(WRITE "${checksum_path}" "${archive_sha256}  ${archive_name}\n")
file(WRITE "${manifest_path}"
    "{\n"
    "  \"name\": \"LingNest\",\n"
    "  \"version\": \"${PROJECT_VERSION}\",\n"
    "  \"platform\": \"windows-x64\",\n"
    "  \"format\": \"portable-zip\",\n"
    "  \"qtVersion\": \"${QT_VERSION}\",\n"
    "  \"builtAtUtc\": \"${build_timestamp}\",\n"
    "  \"archive\": \"${archive_name}\",\n"
    "  \"archiveBytes\": ${archive_size},\n"
    "  \"archiveSha256\": \"${archive_sha256}\",\n"
    "  \"fileCount\": ${packaged_file_count}\n"
    "}\n")

message(STATUS "Windows package created: ${archive_path}")
message(STATUS "SHA-256: ${archive_sha256}")
