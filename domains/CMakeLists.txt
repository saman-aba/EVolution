add_library(
    evolution_domain
    src/contracts.cpp
)
add_library(EVolution::domain ALIAS evolution_domain)

target_compile_features(evolution_domain PUBLIC cxx_std_20)
target_link_libraries(evolution_domain PUBLIC EVolution::core EVolution::extensions)
target_include_directories(
    evolution_domain
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

evolution_set_warnings(evolution_domain)
evolution_enable_sanitizers(evolution_domain)

set_target_properties(
    evolution_domain
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME domain
)

install(
    TARGETS evolution_domain
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

add_subdirectory(poker)
