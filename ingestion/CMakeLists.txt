add_library(evolution_ingestion src/ingestion.cpp)
add_library(EVolution::ingestion ALIAS evolution_ingestion)

target_compile_features(evolution_ingestion PUBLIC cxx_std_20)
target_link_libraries(evolution_ingestion PUBLIC EVolution::core EVolution::security)
target_include_directories(
    evolution_ingestion
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

evolution_set_warnings(evolution_ingestion)
evolution_enable_sanitizers(evolution_ingestion)

set_target_properties(
    evolution_ingestion
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME ingestion
)

install(
    TARGETS evolution_ingestion
    EXPORT EVolutionTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.hpp")
