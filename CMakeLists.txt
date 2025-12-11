cmake_minimum_required(VERSION 3.10)
project(MyBitcoin)

set(CMAKE_CXX_STANDARD 17)

# =======================================================
# 1. 设置 OpenSSL 根目录 (根据你的截图路径)
# =======================================================
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")

# 2. 寻找 OpenSSL 库
find_package(OpenSSL REQUIRED)

# 3. 包含项目自身的头文件路径
include_directories(src)

# =======================================================
# 4. [关键修复] 包含 OpenSSL 的头文件路径
# 如果没有这一行，编译器就找不到 <openssl/sha.h>
# =======================================================
include_directories(${OPENSSL_INCLUDE_DIR})

# 5. 收集源码
file(GLOB_RECURSE SRC_FILES "src/Crypto/*.cpp")

# 6. 创建测试可执行文件
add_executable(test_crypto tests/test_crypto.cpp ${SRC_FILES})

# 7. 链接 OpenSSL
target_link_libraries(test_crypto OpenSSL::SSL OpenSSL::Crypto)