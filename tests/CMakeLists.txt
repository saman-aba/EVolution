function(evolution_add_test target category source)
    add_executable(${target} "${source}")
    target_link_libraries(${target} PRIVATE ${ARGN})
    evolution_set_warnings(${target})
    evolution_enable_sanitizers(${target})

    add_test(NAME "${category}.${target}" COMMAND ${target})
    set_tests_properties("${category}.${target}" PROPERTIES LABELS "${category}")
endfunction()

evolution_add_test(
    evolution_core_unit_tests
    unit
    unit/core_foundation_test.cpp
    EVolution::core
)

evolution_add_test(
    evolution_core_information_tests
    unit
    unit/core_information_test.cpp
    EVolution::core
)

evolution_add_test(
    evolution_module_integration_tests
    integration
    integration/module_graph_test.cpp
    EVolution::analysis
    EVolution::domain_poker
    EVolution::ingestion
    EVolution::storage
)

evolution_add_test(
    evolution_processing_contract_tests
    unit
    unit/processing_contract_test.cpp
    EVolution::processing
)

evolution_add_test(
    evolution_processing_graph_tests
    unit
    unit/processing_graph_test.cpp
    EVolution::processing
)

evolution_add_test(
    evolution_processing_recovery_tests
    unit
    unit/processing_recovery_test.cpp
    EVolution::processing
)

evolution_add_test(
    evolution_storage_contract_tests
    unit
    unit/storage_contract_test.cpp
    EVolution::storage
)

evolution_add_test(
    evolution_extension_domain_tests
    unit
    unit/extension_domain_test.cpp
    EVolution::extensions
    EVolution::domain
    EVolution::domain_poker
)

evolution_add_test(
    evolution_system_tests
    system
    system/composition_test.cpp
    EVolution::interfaces
)

if(EVOLUTION_BUILD_BENCHMARKS)
    add_executable(evolution_build_benchmark benchmarks/build_baseline.cpp)
    target_link_libraries(evolution_build_benchmark PRIVATE EVolution::interfaces)
    evolution_set_warnings(evolution_build_benchmark)
    evolution_enable_sanitizers(evolution_build_benchmark)
endif()
