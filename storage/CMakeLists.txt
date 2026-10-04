add_library(
    evolution_storage
    src/contracts.cpp
    src/query.cpp
    src/read_model.cpp
)
add_library(EVolution::storage ALIAS evolution_storage)

target_compile_features(evolution_storage PUBLIC cxx_std_20)
target_link_libraries(evolution_storage PUBLIC EVolution::core)
target_include_directories(
    evolution_storage
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

evolution_set_warnings(evolution_storage)
evolution_enable_sanitizers(evolution_storage)

set_target_properties(
    evolution_storage
    PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN YES
        EXPORT_NAME storage
)

install(
    TARGETS evolution_storage
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
