add_library(
    evolution_processing
    src/checkpoint.cpp
    src/delivery.cpp
    src/execution.cpp
    src/graph.cpp
    src/lifecycle.cpp
    src/observability.cpp
    src/recovery.cpp
    src/reproducibility.cpp
    src/resources.cpp
    src/scheduler.cpp
    src/supervision.cpp
    src/validation.cpp
)
add_library(EVolution::processing ALIAS evolution_processing)

target_compile_features(evolution_processing PUBLIC cxx_std_20)
target_link_libraries(evolution_processing PUBLIC EVolution::core)
target_include_directories(
    evolution_processing
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

evolution_set_warnings(evolution_processing)
evolution_enable_sanitizers(evolution_processing)

set_target_properties(
    evolution_processing
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME processing
)

install(
    TARGETS evolution_processing
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
