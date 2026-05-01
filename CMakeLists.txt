cmake_minimum_required(VERSION 3.15)
project(LimitOrderBook)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
)

set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

enable_testing()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
