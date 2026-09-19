cmake_minimum_required(VERSION 3.21)

foreach(required_variable IN ITEMS
        SOURCE_DIR
        PACKAGE_ROOT
        PORTABLE_BASENAME
        INSTALLER_BASENAME
        PROJECT_VERSION)
    if(NOT DEFINED ${required_variable} OR "${${required_variable}}" STREQUAL "")
        message(FATAL_ERROR "${required_variable} must be provided")
    endif()
endforeach()

get_filename_component(source_directory "${SOURCE_DIR}" ABSOLUTE)
set(distribution_directory "${source_directory}/dist")
get_filename_component(distribution_parent "${distribution_directory}" DIRECTORY)
get_filename_component(distribution_name "${distribution_directory}" NAME)

if(NOT distribution_parent STREQUAL source_directory
        OR NOT distribution_name STREQUAL "dist")
    message(FATAL_ERROR
        "Refusing to replace an unexpected distribution path: ${distribution_directory}")
endif()

set(installer_path "${PACKAGE_ROOT}/${INSTALLER_BASENAME}.exe")
set(portable_zip_path "${PACKAGE_ROOT}/${PORTABLE_BASENAME}.zip")
set(portable_checksum_path "${PACKAGE_ROOT}/${PORTABLE_BASENAME}.sha256")
set(portable_manifest_path "${PACKAGE_ROOT}/${PORTABLE_BASENAME}.manifest.json")

foreach(required_file IN ITEMS
        "${installer_path}"
        "${portable_zip_path}"
        "${portable_checksum_path}"
        "${portable_manifest_path}")
    if(NOT EXISTS "${required_file}")
        message(FATAL_ERROR "Release artifact is missing: ${required_file}")
    endif()
endforeach()

# dist is a generated, source-root-local delivery directory. Replacing this exact
# directory prevents stale versioned artifacts from being mistaken for this release.
file(REMOVE_RECURSE "${distribution_directory}")
file(MAKE_DIRECTORY "${distribution_directory}")

foreach(release_file IN ITEMS
        "${installer_path}"
        "${portable_zip_path}"
        "${portable_checksum_path}"
        "${portable_manifest_path}")
    file(COPY "${release_file}" DESTINATION "${distribution_directory}")
endforeach()

file(SHA256 "${installer_path}" installer_sha256)
file(SIZE "${installer_path}" installer_size)
get_filename_component(installer_name "${installer_path}" NAME)
file(WRITE "${distribution_directory}/${INSTALLER_BASENAME}.sha256"
    "${installer_sha256}  ${installer_name}\n")

file(WRITE "${distribution_directory}/release-manifest.json"
    "{\n"
    "  \"name\": \"LingNest\",\n"
    "  \"version\": \"${PROJECT_VERSION}\",\n"
    "  \"platform\": \"windows-x64\",\n"
    "  \"recommendedArtifact\": \"${installer_name}\",\n"
    "  \"installerBytes\": ${installer_size},\n"
    "  \"installerSha256\": \"${installer_sha256}\",\n"
    "  \"portableArtifact\": \"${PORTABLE_BASENAME}.zip\"\n"
    "}\n")

file(WRITE "${distribution_directory}/README-先看.txt"
    "LingNest v${PROJECT_VERSION} - Windows x64\n"
    "\n"
    "推荐安装方式：\n"
    "  双击 ${installer_name}。安装程序已包含 Qt、Qt 插件、角色资源和 VC++ 运行库。\n"
    "\n"
    "便携方式：\n"
    "  将 ${PORTABLE_BASENAME}.zip 完整解压后，再运行解压目录中的 LingNest.exe。\n"
    "  不要在压缩包预览窗口中直接运行，也不要只复制 LingNest.exe。\n"
    "\n"
    "重要：\n"
    "  build/release/LingNest.exe 是开发构建产物，不是可分发软件包。\n"
    "  对外分发时只使用本 dist 目录中的 setup.exe 或完整 portable.zip。\n")

message(STATUS "Distribution directory created: ${distribution_directory}")
message(STATUS "Installer SHA-256: ${installer_sha256}")
