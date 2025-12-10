cmake_minimum_required(VERSION 3.10)
project(MyBitcoin)

set(CMAKE_CXX_STANDARD 17)

# 寻找 OpenSSL 库
find_package(OpenSSL REQUIRED)

# 包含头文件路径
include_directories(src)

# 收集源码
file(GLOB_RECURSE SRC_FILES "src/Crypto/*.cpp")

# 创建测试可执行文件
add_executable(test_crypto tests/test_crypto.cpp ${SRC_FILES})

# 链接 OpenSSL
target_link_libraries(test_crypto OpenSSL::SSL OpenSSL::Crypto)