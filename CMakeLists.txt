cmake_minimum_required(VERSION 3.15)
project(LimitOrderBook)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(LimitOrderBook
    main.cpp
    OrderBook.cpp
)