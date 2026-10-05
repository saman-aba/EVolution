add_library(
    evolution_extensions
    src/contracts.cpp
    src/registry.cpp
)
add_library(EVolution::extensions ALIAS evolution_extensions)

target_compile_features(evolution_extensions PUBLIC cxx_std_20)
target_link_libraries(evolution_extensions PUBLIC EVolution::core)
target_include_directories(
    evolution_extensions
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

evolution_set_warnings(evolution_extensions)
evolution_enable_sanitizers(evolution_extensions)

set_target_properties(
    evolution_extensions
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME extensions
)

install(
    TARGETS evolution_extensions
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
