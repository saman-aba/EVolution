cmake_minimum_required(VERSION 3.25)

project(
    EVolution
    VERSION 0.1.0
    DESCRIPTION "Event-driven analytical platform"
    LANGUAGES CXX
)

include(CTest)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(EVOLUTION_BUILD_TESTS "Build EVolution tests" ${BUILD_TESTING})
option(EVOLUTION_BUILD_TOOLS "Build EVolution developer tools" ON)
option(EVOLUTION_BUILD_EXAMPLES "Build EVolution examples" OFF)
option(EVOLUTION_BUILD_BENCHMARKS "Build EVolution benchmarks" OFF)
option(EVOLUTION_WARNINGS_AS_ERRORS "Treat EVolution warnings as errors" OFF)
option(EVOLUTION_ENABLE_ADDRESS_SANITIZER "Enable AddressSanitizer" OFF)
option(EVOLUTION_ENABLE_UNDEFINED_SANITIZER "Enable UndefinedBehaviorSanitizer" OFF)
option(EVOLUTION_ENABLE_CLANG_TIDY "Run clang-tidy during compilation" OFF)

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(EvolutionWarnings)
include(EvolutionSanitizers)
include(EvolutionStaticAnalysis)
include(EvolutionFormat)

evolution_configure_static_analysis()
evolution_add_format_targets()

add_subdirectory(core)
add_subdirectory(processing)
add_subdirectory(extensions)
add_subdirectory(domains)
add_subdirectory(analysis)
add_subdirectory(storage)
add_subdirectory(ingestion)
add_subdirectory(applications)
add_subdirectory(interfaces)

if(EVOLUTION_BUILD_TOOLS)
    add_subdirectory(tools)
endif()

if(EVOLUTION_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/EVolutionConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/EVolutionConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/EVolutionConfig.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/EVolution"
)

install(
    EXPORT EVolutionTargets
    FILE EVolutionTargets.cmake
    NAMESPACE EVolution::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/EVolution"
)

install(
    FILES
        "${CMAKE_CURRENT_BINARY_DIR}/EVolutionConfig.cmake"
        "${CMAKE_CURRENT_BINARY_DIR}/EVolutionConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/EVolution"
)
