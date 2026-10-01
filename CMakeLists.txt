cmake_minimum_required(VERSION 3.14)
project(AAII VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(SDL3 REQUIRED)
find_package(Threads REQUIRED)

# Add the parser library
add_subdirectory(PARSER)

# Add GUI application (optional, can be disabled for pure library build)
option(BUILD_GUI "Build the GUI application" ON)
if(BUILD_GUI)
    add_subdirectory(GUI)
endif()

# Install rules
install(TARGETS AAII PARSER LIBRARY DESTINATION lib)
