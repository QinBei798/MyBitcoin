cmake_minimum_required(VERSION 3.10)
project(MyBitcoin)

set(CMAKE_CXX_STANDARD 17)

# 1. 设置 OpenSSL 路径
set(OPENSSL_ROOT_DIR "C:/Program Files/OpenSSL-Win64")

# 2. 寻找库
find_package(OpenSSL REQUIRED)

# 3. 包含路径
include_directories(src)
include_directories(${OPENSSL_INCLUDE_DIR})

# =======================================================
# 4. [关键修正] 一次性搜集所有核心源码
# 包含了 Crypto, Core, 和 Wallet 下所有的 .cpp 文件
# 这样你就不用在后面手动写 "src/Wallet/Wallet.cpp" 了
# =======================================================
file(GLOB_RECURSE SRC_FILES 
    "src/Crypto/*.cpp" 
    "src/Core/*.cpp" 
    "src/Wallet/*.cpp"
)

# 5. 定义自动复制 DLL 函数 (保持不变)
function(auto_copy_openssl_dlls target_name)
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
# 6. 定义可执行文件 (独立拆分)
# =======================================================

# --- 目标 1: Crypto 测试 (只包含 crypto 测试代码 + 核心库) ---
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_crypto.cpp")
    add_executable(test_crypto tests/test_crypto.cpp ${SRC_FILES})
    target_link_libraries(test_crypto OpenSSL::SSL OpenSSL::Crypto)
    auto_copy_openssl_dlls(test_crypto)
endif()

# --- 目标 2: Block 测试 (只包含 block 测试代码 + 核心库) ---
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_block.cpp")
    add_executable(test_block tests/test_block.cpp ${SRC_FILES})
    target_link_libraries(test_block OpenSSL::SSL OpenSSL::Crypto)
    auto_copy_openssl_dlls(test_block)
endif()

# --- 目标 3: Wallet 测试 (这是你今天的新目标) ---
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_wallet.cpp")
    # 注意：这里只编译 tests/test_wallet.cpp，不要把其他 test_xxx.cpp 加进来
    add_executable(test_wallet tests/test_wallet.cpp ${SRC_FILES})
    target_link_libraries(test_wallet OpenSSL::SSL OpenSSL::Crypto)
    auto_copy_openssl_dlls(test_wallet)
endif()

# --- 目标 3: Transaction 测试  ---
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_transaction.cpp")
    add_executable(test_transaction tests/test_transaction.cpp ${SRC_FILES})
    target_link_libraries(test_transaction OpenSSL::SSL OpenSSL::Crypto)
    auto_copy_openssl_dlls(test_transaction)
endif()

# 新增 test_blockchain 目标
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_blockchain.cpp")
    add_executable(test_blockchain tests/test_blockchain.cpp ${SRC_FILES})
    target_link_libraries(test_blockchain OpenSSL::SSL OpenSSL::Crypto)
    auto_copy_openssl_dlls(test_blockchain)
endif() 

# 新增 test_persistence 目标
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_persistence.cpp")
    add_executable(test_persistence tests/test_persistence.cpp ${SRC_FILES})
    target_link_libraries(test_persistence OpenSSL::SSL OpenSSL::Crypto)
    auto_copy_openssl_dlls(test_persistence)
endif() 

# --- 新增: 钱包持久化测试 ---
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_wallet_persistence.cpp")
    add_executable(test_wallet_p tests/test_wallet_persistence.cpp ${SRC_FILES})
    target_link_libraries(test_wallet_p OpenSSL::SSL OpenSSL::Crypto)
    auto_copy_openssl_dlls(test_wallet_p)
endif()

# 动态难度测试
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_difficulty.cpp")
    add_executable(test_difficulty tests/test_difficulty.cpp ${SRC_FILES})
    target_link_libraries(test_difficulty OpenSSL::SSL OpenSSL::Crypto)
    auto_copy_openssl_dlls(test_difficulty)
endif()

# --- 新增: 钱包交易构造测试 (验证找零) ---
if(EXISTS "${CMAKE_SOURCE_DIR}/tests/test_wallet_tx.cpp")
    add_executable(test_wallet_tx tests/test_wallet_tx.cpp ${SRC_FILES})
    target_link_libraries(test_wallet_tx OpenSSL::SSL OpenSSL::Crypto)
    auto_copy_openssl_dlls(test_wallet_tx)
endif()