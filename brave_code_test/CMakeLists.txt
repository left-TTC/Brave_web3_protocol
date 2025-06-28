cmake_minimum_required(VERSION 3.10)
project(Web3ServiceProject)

#path to the floder where the libsodium is located 
set(LIBSODIUM_BASE /home/f)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

find_package(CURL REQUIRED)

set(SOURCES
    main.cpp
    brave_web3_service.cc
    brave_web3_rpc.cc
)

add_executable(test ${SOURCES})

target_include_directories(test PRIVATE
    ${CURL_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/cppcodec     
    ${CMAKE_SOURCE_DIR}    
    ${LIBSODIUM_BASE}/libsodium-stable/src/libsodium/include           
)

target_link_libraries(test PRIVATE
    ${LIBSODIUM_BASE}/libsodium-stable/src/libsodium/.libs/libsodium.a
    CURL::libcurl
)
