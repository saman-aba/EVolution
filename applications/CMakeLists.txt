add_library(
    evolution_applications
    src/lifecycle.cpp
    src/operations.cpp
)
add_library(EVolution::applications ALIAS evolution_applications)

target_compile_features(evolution_applications PUBLIC cxx_std_20)
target_link_libraries(
    evolution_applications
    PUBLIC
        EVolution::analysis
        EVolution::domain_poker
        EVolution::extensions
        EVolution::ingestion
        EVolution::processing
        EVolution::security
        EVolution::storage
)
target_include_directories(
    evolution_applications
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

evolution_set_warnings(evolution_applications)
evolution_enable_sanitizers(evolution_applications)

set_target_properties(
    evolution_applications
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME applications
)

install(
    TARGETS evolution_applications
    EXPORT EVolutionTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")
