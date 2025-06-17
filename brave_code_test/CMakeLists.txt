cmake_minimum_required(VERSION 3.10)  
project(Web3ServiceProject)  

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SOURCES
    main.cpp
    brave_web3_service.cc
    brave_web3_service.h
)

add_executable(test ${SOURCES})

