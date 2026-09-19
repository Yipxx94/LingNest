if(NOT DEFINED SOURCE_DIR OR NOT IS_DIRECTORY "${SOURCE_DIR}")
    message(FATAL_ERROR "SOURCE_DIR must point to the character source directory")
endif()

if(NOT DEFINED DESTINATION_DIR OR DESTINATION_DIR STREQUAL "")
    message(FATAL_ERROR "DESTINATION_DIR must be provided")
endif()

file(REMOVE_RECURSE "${DESTINATION_DIR}")
file(MAKE_DIRECTORY "${DESTINATION_DIR}")

file(GLOB character_directories LIST_DIRECTORIES true "${SOURCE_DIR}/*")
set(staged_character_count 0)

foreach(character_directory IN LISTS character_directories)
    if(NOT IS_DIRECTORY "${character_directory}")
        continue()
    endif()

    if(NOT EXISTS "${character_directory}/character.json")
        continue()
    endif()

    get_filename_component(character_id "${character_directory}" NAME)
    set(character_destination "${DESTINATION_DIR}/${character_id}")
    file(MAKE_DIRECTORY "${character_destination}")

    file(COPY "${character_directory}/character.json"
        DESTINATION "${character_destination}")

    if(EXISTS "${character_directory}/prompt.md")
        file(COPY "${character_directory}/prompt.md"
            DESTINATION "${character_destination}")
    endif()

    if(EXISTS "${character_directory}/assets")
        file(COPY "${character_directory}/assets"
            DESTINATION "${character_destination}")
    endif()

    math(EXPR staged_character_count "${staged_character_count} + 1")
endforeach()

if(staged_character_count EQUAL 0)
    message(FATAL_ERROR "No runtime character packages were found in ${SOURCE_DIR}")
endif()

message(STATUS "Staged ${staged_character_count} character package(s)")
