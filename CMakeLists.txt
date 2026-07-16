cmake_minimum_required(VERSION 3.15)
project(LimitOrderBook
    VERSION 2.2.0
    LANGUAGES CXX
)

add_library(orderbook STATIC
    src/OrderBook.cpp
    src/Order.cpp
)

add_library(orderbook::orderbook ALIAS orderbook)

target_include_directories(orderbook
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

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

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

install(TARGETS orderbook
    EXPORT orderbookTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(DIRECTORY src/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h"
)

install(EXPORT orderbookTargets
    FILE orderbookTargets.cmake
    NAMESPACE orderbook::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/orderbook
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/orderbookConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

configure_file(cmake/orderbookConfig.cmake.in
    "${CMAKE_CURRENT_BINARY_DIR}/orderbookConfig.cmake"
    @ONLY
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/orderbookConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/orderbookConfigVersion.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/orderbook
)

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

    set(TEST_SOURCES
        tests/order_tests.cpp
        tests/orderbook_basic_tests.cpp
        tests/matching_tests.cpp
        tests/market_order_tests.cpp
        tests/ioc_tests.cpp
        tests/fok_tests.cpp
    )

    add_executable(orderbook_tests ${TEST_SOURCES})
    target_link_libraries(orderbook_tests PRIVATE orderbook GTest::gtest_main)
    include(GoogleTest)
    gtest_discover_tests(orderbook_tests)

    set(BENCH_SOURCES
        bench/add_order_bench.cpp
        bench/cancel_order_bench.cpp
        bench/depth_bench.cpp
        bench/level_info_bench.cpp
    )

    foreach(BENCH_SRC ${BENCH_SOURCES})
        get_filename_component(BENCH_NAME ${BENCH_SRC} NAME_WE)
        add_executable(${BENCH_NAME} ${BENCH_SRC})
        target_link_libraries(${BENCH_NAME} PRIVATE orderbook benchmark::benchmark)
        target_include_directories(${BENCH_NAME} PRIVATE bench)
        if(ipo_ok)
            set_target_properties(${BENCH_NAME} PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        endif()
        if(MSVC)
            target_compile_options(${BENCH_NAME} PRIVATE
                $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /arch:AVX2>
            )
        endif()
    endforeach()

    add_executable(orderbook_bench_all
        bench/add_order_bench.cpp
        bench/cancel_order_bench.cpp
        bench/depth_bench.cpp
        bench/level_info_bench.cpp
    )
    target_link_libraries(orderbook_bench_all PRIVATE orderbook benchmark::benchmark_main)
    target_include_directories(orderbook_bench_all PRIVATE bench)
    target_compile_definitions(orderbook_bench_all PRIVATE BENCHMARK_COMBINED)
    if(ipo_ok)
        set_target_properties(orderbook_bench_all PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
    endif()
    if(MSVC)
        target_compile_options(orderbook_bench_all PRIVATE
            $<$<CONFIG:Release>:/O2 /Ob2 /Oi /Ot /arch:AVX2>
        )
    endif()
endif()
