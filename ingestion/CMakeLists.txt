add_library(evolution_ingestion INTERFACE)
add_library(EVolution::ingestion ALIAS evolution_ingestion)

target_compile_features(evolution_ingestion INTERFACE cxx_std_20)
target_link_libraries(evolution_ingestion INTERFACE EVolution::core)

set_target_properties(evolution_ingestion PROPERTIES EXPORT_NAME ingestion)

install(TARGETS evolution_ingestion EXPORT EVolutionTargets)
