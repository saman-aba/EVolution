add_library(evolution_applications INTERFACE)
add_library(EVolution::applications ALIAS evolution_applications)

target_compile_features(evolution_applications INTERFACE cxx_std_20)
target_link_libraries(
    evolution_applications
    INTERFACE
        EVolution::analysis
        EVolution::domain_poker
        EVolution::extensions
        EVolution::ingestion
        EVolution::processing
        EVolution::storage
)

set_target_properties(evolution_applications PROPERTIES EXPORT_NAME applications)

install(TARGETS evolution_applications EXPORT EVolutionTargets)
