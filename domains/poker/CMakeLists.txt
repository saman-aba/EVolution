add_library(
    evolution_domain_poker
    src/domain.cpp
    src/model.cpp
)
add_library(EVolution::domain_poker ALIAS evolution_domain_poker)

target_compile_features(evolution_domain_poker PUBLIC cxx_std_20)
target_link_libraries(
    evolution_domain_poker
    PUBLIC
        EVolution::core
        EVolution::domain
        EVolution::extensions
)
target_include_directories(
    evolution_domain_poker
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

evolution_set_warnings(evolution_domain_poker)
evolution_enable_sanitizers(evolution_domain_poker)

set_target_properties(
    evolution_domain_poker
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME domain_poker
)

install(
    TARGETS evolution_domain_poker
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
