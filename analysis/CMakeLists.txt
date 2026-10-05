add_library(
    evolution_analysis
    src/aggregation.cpp
    src/analysis.cpp
    src/common.cpp
    src/decision.cpp
    src/pattern.cpp
)
add_library(EVolution::analysis ALIAS evolution_analysis)

target_compile_features(evolution_analysis PUBLIC cxx_std_20)
target_link_libraries(evolution_analysis PUBLIC EVolution::core EVolution::processing)
target_include_directories(
    evolution_analysis
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

evolution_set_warnings(evolution_analysis)
evolution_enable_sanitizers(evolution_analysis)

set_target_properties(
    evolution_analysis
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME analysis
)

install(
    TARGETS evolution_analysis
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
