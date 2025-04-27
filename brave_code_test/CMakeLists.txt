cmake_minimum_required(VERSION 3.10)  
project(Web3ServiceProject)  

find_package(OpenSSL REQUIRED)
find_package(Sodium REQUIRED)


set(CMAKE_CXX_STANDARD 11)

set(SOURCES
    main.cpp
    brave_web3_service.cc
    brave_web3_service.h
)

add_executable(test ${SOURCES})

target_link_libraries(test OpenSSL::SSL OpenSSL::Crypto)
target_link_libraries(test Sodium::Sodium)
