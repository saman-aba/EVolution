function(evolution_add_format_targets)
    find_program(EVOLUTION_CLANG_FORMAT_EXECUTABLE NAMES clang-format)
    if(NOT EVOLUTION_CLANG_FORMAT_EXECUTABLE)
        return()
    endif()

    file(
        GLOB_RECURSE EVOLUTION_FORMAT_FILES CONFIGURE_DEPENDS
        "${PROJECT_SOURCE_DIR}/*.c"
        "${PROJECT_SOURCE_DIR}/*.cc"
        "${PROJECT_SOURCE_DIR}/*.cpp"
        "${PROJECT_SOURCE_DIR}/*.h"
        "${PROJECT_SOURCE_DIR}/*.hh"
        "${PROJECT_SOURCE_DIR}/*.hpp"
    )
    list(FILTER EVOLUTION_FORMAT_FILES EXCLUDE REGEX "^${PROJECT_SOURCE_DIR}/build([/-]|$)")

    if(NOT EVOLUTION_FORMAT_FILES)
        return()
    endif()

    add_custom_target(
        format
        COMMAND "${EVOLUTION_CLANG_FORMAT_EXECUTABLE}" -i ${EVOLUTION_FORMAT_FILES}
        COMMENT "Formatting EVolution C/C++ sources"
        VERBATIM
    )

    add_custom_target(
        format-check
        COMMAND "${EVOLUTION_CLANG_FORMAT_EXECUTABLE}" --dry-run --Werror ${EVOLUTION_FORMAT_FILES}
        COMMENT "Checking EVolution C/C++ formatting"
        VERBATIM
    )
endfunction()
