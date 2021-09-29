cmake_minimum_required(VERSION 3.17)
project(performance_summit_202109_benchmarks LANGUAGES C CXX ASM)
set (CMAKE_CXX_STANDARD 20)

if (CMAKE_BUILD_TYPE MATCHES Debug)
    add_definitions(-DDEBUG=1)

    add_compile_options($<$<COMPILE_LANGUAGE:C>:-O0>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-O0>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-fno-inline>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-inline>)
    add_compile_options($<$<COMPILE_LANGUAGE:C>:-fno-omit-frame-pointer>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-omit-frame-pointer>)

    message(STATUS "Debug build")
elseif (CMAKE_BUILD_TYPE MATCHES Release)
    add_definitions(-DNDEBUG=1)

    add_compile_options($<$<COMPILE_LANGUAGE:C>:-O3>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-O3>)

    message(STATUS "Release build")
endif()

add_subdirectory(libfiber)

include(ExternalProject)

ExternalProject_Add(benchmark-install
        PREFIX benchmark-sources
        INSTALL_DIR ${CMAKE_BINARY_DIR}/benchmark-install
        URL https://github.com/google/benchmark/archive/v1.6.0.tar.gz
        CMAKE_CACHE_ARGS
        -DCMAKE_C_COMPILER:STRING=${CMAKE_C_COMPILER}
        -DCMAKE_CXX_COMPILER:STRING=${CMAKE_CXX_COMPILER}
        -DCMAKE_BUILD_TYPE:STRING=RELEASE
        -DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
        -DCMAKE_CXX_FLAGS:STRING=${BENCHMARK_LIBCXX_COMPILE_FLAGS}
        -DBENCHMARK_ENABLE_TESTING:BOOL=OFF
        -DBENCHMARK_DOWNLOAD_DEPENDENCIES:BOOL=ON)

add_library(benchmark::benchmark STATIC IMPORTED)
add_dependencies(benchmark::benchmark benchmark-install)
set_target_properties(benchmark::benchmark PROPERTIES
        IMPORTED_LINK_INTERFACE_LANGUAGES "CXX"
        IMPORTED_LOCATION "${CMAKE_BINARY_DIR}/benchmark-install/lib/libbenchmark.a"
        )

# Build benches
file(GLOB SRC_BENCHES "bench-*.cpp")

add_executable(
        performance_summit_202109_benchmarks
        main.cpp ${SRC_BENCHES})

add_dependencies(
        performance_summit_202109_benchmarks
        fiber)

target_compile_options(
        performance_summit_202109_benchmarks
        PRIVATE
        -mclflushopt -march=native)

set_target_properties(
        performance_summit_202109_benchmarks
        PROPERTIES
        LINKER_LANGUAGE CXX)

target_link_libraries(
        performance_summit_202109_benchmarks
        PRIVATE
        benchmark::benchmark pthread fiber)

target_include_directories(
        performance_summit_202109_benchmarks
        PRIVATE
        "${CMAKE_BINARY_DIR}/benchmark-install/include"
)
