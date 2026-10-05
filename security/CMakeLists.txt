add_library(evolution_security src/security.cpp)
add_library(EVolution::security ALIAS evolution_security)

target_compile_features(evolution_security PUBLIC cxx_std_20)
target_link_libraries(evolution_security PUBLIC EVolution::core)
target_include_directories(
    evolution_security
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

evolution_set_warnings(evolution_security)
evolution_enable_sanitizers(evolution_security)

set_target_properties(
    evolution_security
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME security
)

install(
    TARGETS evolution_security
    EXPORT EVolutionTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")
