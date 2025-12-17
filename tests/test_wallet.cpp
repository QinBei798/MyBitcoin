#include "../src/Wallet/Wallet.h"
#include <iostream>
#include <cassert>
#include <cstdio> // for remove
#include <openssl/applink.c> 

void TestWallet() {
    std::string testFile = "test_wallet_basic.dat";
    // 先删除旧文件，确保每次测试都是新的
    remove(testFile.c_str());

    std::cout << "Generating new wallet..." << std::endl;

    // [修改] 构造函数会自动调用 GenerateNewKey，不需要也不允许手动调用
    Wallet myWallet(testFile);

    // 1. 获取公钥
    Bytes pubKey = myWallet.GetPublicKey();
    std::cout << "Public Key (" << pubKey.size() << " bytes): " << ToHex(pubKey) << std::endl;
    // 压缩公钥应该是 33 字节，以 02 或 03 开头
    assert(pubKey.size() == 33);

    // 2. 获取地址
    std::string address = myWallet.GetAddress();
    std::cout << "Bitcoin Address: " << address << std::endl;

    // 验证地址特征
    assert(address[0] == '1');
    assert(address.length() > 20);

    std::cout << "Wallet Test Passed!" << std::endl;
}

int main() {
    try {
        TestWallet();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}