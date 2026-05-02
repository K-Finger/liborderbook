cmake_minimum_required(VERSION 3.15)
project(LimitOrderBook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

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

enable_testing()

add_executable(LimitOrderBook
    main.cpp
    OrderBook.cpp
    Order.cpp
)

add_executable(orderbook_tests
    OrderBookTests.cpp
    OrderBook.cpp
    Order.cpp
)
target_link_libraries(orderbook_tests PRIVATE GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(orderbook_tests)

add_executable(orderbook_bench
    OrderBookBench.cpp
    OrderBook.cpp
    Order.cpp
)
target_link_libraries(orderbook_bench PRIVATE benchmark::benchmark_main)

include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_ok)
set_target_properties(orderbook_bench PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION_RELEASE ${ipo_ok}
)

if(MSVC)
    target_compile_options(orderbook_bench PRIVATE
        $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /arch:AVX2>
    )
endif()
