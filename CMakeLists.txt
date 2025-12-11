cmake_minimum_required(VERSION 3.10)
project(MyBitcoin)

set(CMAKE_CXX_STANDARD 17)

# =======================================================
# 1. 路径设置 (根据你的截图，DLL在根目录下)
# =======================================================
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")

# 2. 寻找库
find_package(OpenSSL REQUIRED)

# 3. 包含路径
include_directories(src)
include_directories(${OPENSSL_INCLUDE_DIR})

# 4. 搜集源码 (同时包含 Crypto 和 Core)
file(GLOB_RECURSE SRC_FILES "src/Crypto/*.cpp" "src/Core/*.cpp")

# =======================================================
# [核心魔法] 定义一个自动复制 DLL 的函数
# =======================================================
function(auto_copy_openssl_dlls target_name)
    # 你的截图显示 DLL 在根目录，所以路径是 ${OPENSSL_ROOT_DIR}
    # 如果未来报错找不到，可以试着改成 ${OPENSSL_ROOT_DIR}/bin
    set(DLL_PATH ${OPENSSL_ROOT_DIR}) 

    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_PATH}/libcrypto-3-x64.dll"
            $<TARGET_FILE_DIR:${target_name}>
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${DLL_PATH}/libssl-3-x64.dll"
            $<TARGET_FILE_DIR:${target_name}>
        COMMENT "正在自动搬运 OpenSSL DLL 到输出目录..."
    )
endfunction()

# =======================================================
# 5. 定义可执行文件
# =======================================================

# --- 目标 A: test_crypto ---
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_crypto.cpp")
    add_executable(test_crypto tests/test_crypto.cpp ${SRC_FILES})
    target_link_libraries(test_crypto OpenSSL::SSL OpenSSL::Crypto)
    # 应用自动复制魔法
    auto_copy_openssl_dlls(test_crypto)
endif()

# --- 目标 B: test_block (你现在正在做的) ---
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_block.cpp")
    add_executable(test_block tests/test_block.cpp ${SRC_FILES})
    target_link_libraries(test_block OpenSSL::SSL OpenSSL::Crypto)
    # 应用自动复制魔法
    auto_copy_openssl_dlls(test_block)
endif()