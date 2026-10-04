add_library(evolution_analysis INTERFACE)
add_library(EVolution::analysis ALIAS evolution_analysis)

target_compile_features(evolution_analysis INTERFACE cxx_std_20)
target_link_libraries(evolution_analysis INTERFACE EVolution::core EVolution::processing)

set_target_properties(evolution_analysis PROPERTIES EXPORT_NAME analysis)

install(TARGETS evolution_analysis EXPORT EVolutionTargets)
