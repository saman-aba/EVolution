add_library(
    evolution_core
    src/configuration.cpp
    src/context.cpp
    src/envelope.cpp
    src/error.cpp
    src/event.cpp
    src/execution_context.cpp
    src/measurement.cpp
    src/provenance.cpp
    src/time.cpp
)
add_library(EVolution::core ALIAS evolution_core)

target_compile_features(evolution_core PUBLIC cxx_std_20)
target_include_directories(
    evolution_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

evolution_set_warnings(evolution_core)
evolution_enable_sanitizers(evolution_core)

set_target_properties(
    evolution_core
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME core
)

install(
    TARGETS evolution_core
    EXPORT EVolutionTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(
    DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING
    PATTERN "*.hpp"
)
