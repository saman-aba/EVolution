function(evolution_configure_static_analysis)
    if(NOT EVOLUTION_ENABLE_CLANG_TIDY)
        return()
    endif()

    find_program(EVOLUTION_CLANG_TIDY_EXECUTABLE NAMES clang-tidy REQUIRED)
    set(CMAKE_CXX_CLANG_TIDY "${EVOLUTION_CLANG_TIDY_EXECUTABLE}" PARENT_SCOPE)
endfunction()
