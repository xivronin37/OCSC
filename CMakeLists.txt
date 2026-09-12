cmake_minimum_required(VERSION 3.10)

project(OCompiler VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

file(GLOB_RECURSE ALL_SOURCES "src/*.cpp")

add_executable(OCompiler ${ALL_SOURCES})

set_target_properties(OCompiler PROPERTIES 
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)


