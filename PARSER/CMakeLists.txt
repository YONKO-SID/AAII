# Bump version here if you change it
set(PARSER_VERSION 0.1.0)

add_library(AAIIParser INTERFACE)
target_include_directories(AAIIParser INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/Include)

# Export the interface targets for easy consumption by other projects
install(TARGETS AAIIParser EXPORT AAIITargets)
install(EXPORT AAIITargets DESTINATION lib/cmake/AAII)
