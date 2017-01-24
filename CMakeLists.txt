cmake_minimum_required(VERSION 3.6)
project(simpler_mptcp_api)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -lcunit")

set(SOURCE_FILES
    mptcp.c
        tests.c
        tests_helpers.c
        tests_helpers.h
        mptcp.h)

add_executable(simpler_mptcp_api ${SOURCE_FILES})