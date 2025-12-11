#include "../src/Wallet/Wallet.h"
#include <iostream>
#include <cassert>

void TestWallet() {
    std::cout << "Generating new wallet..." << std::endl;

    Wallet myWallet;
    myWallet.GenerateNewKey();

    // 1. 获取公钥
    Bytes pubKey = myWallet.GetPublicKey();
    std::cout << "Public Key (" << pubKey.size() << " bytes): " << ToHex(pubKey) << std::endl;
    // 压缩公钥应该是 33 字节，以 02 或 03 开头
    assert(pubKey.size() == 33);

    // 2. 获取地址
    std::string address = myWallet.GetAddress();
    std::cout << "Bitcoin Address: " << address << std::endl;

    // 验证地址特征
    // 主网地址通常以 '1' 开头，长度在 26-35 之间
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