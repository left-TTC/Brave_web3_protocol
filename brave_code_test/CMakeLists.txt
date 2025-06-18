cmake_minimum_required(VERSION 3.10)
project(Web3ServiceProject)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查找 libsodium
find_package(PkgConfig REQUIRED)
pkg_check_modules(SODIUM REQUIRED libsodium)

# 添加源文件
set(SOURCES
    main.cpp
    brave_web3_service.cc
    brave_web3_service.h
    brave_web3_rpc.cc
    brave_web3_rpc.h
    json.hpp
)

# 添加可执行文件
add_executable(test ${SOURCES})

# 包含头文件路径
include_directories(${SODIUM_INCLUDE_DIRS})

# 链接 libsodium 库
target_link_libraries(test ${SODIUM_LIBRARIES})
