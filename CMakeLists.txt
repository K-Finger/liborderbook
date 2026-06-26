cmake_minimum_required(VERSION 3.15)
project(LimitOrderBook LANGUAGES CXX)

# The library. This is what a parent project links against.
add_library(orderbook STATIC
    src/OrderBook.cpp
    src/Order.cpp
)
target_include_directories(orderbook PUBLIC src)
target_compile_features(orderbook PUBLIC cxx_std_20)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_ok)
if(ipo_ok)
    set_target_properties(orderbook PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
endif()

if(MSVC)
    target_compile_options(orderbook PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /arch:AVX2>
    )
endif()

# Tests and benchmarks only build when this is the top-level project, so
# importing the library doesn't pull in GoogleTest / Benchmark.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    include(FetchContent)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        v1.15.2
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)

    FetchContent_Declare(
        benchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG        v1.9.0
    )
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(benchmark)

    add_executable(orderbook_tests tests/OrderBookTests.cpp)
    target_link_libraries(orderbook_tests PRIVATE orderbook GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(orderbook_tests)

    add_executable(orderbook_bench bench/OrderBookBench.cpp)
    target_link_libraries(orderbook_bench PRIVATE orderbook benchmark::benchmark_main)
    if(ipo_ok)
        set_target_properties(orderbook_bench PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    endif()
    if(MSVC)
        target_compile_options(orderbook_bench PRIVATE
            $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /arch:AVX2>
        )
    endif()
endif()
