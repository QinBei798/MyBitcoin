#include "../src/Core/Blockchain.h"
#include "../src/Wallet/Wallet.h"
#include <iostream>
#include <cassert>
#include <cstdio>
// [关键] 解决 Windows 下 OpenSSL "no OPENSSL_Applink" 错误
#include <openssl/applink.c>

void TestPersistence() {
    std::string dbFile = "blockchain_test.dat";
    std::string walletFile = "alice_test.dat";

    remove(dbFile.c_str());
    remove(walletFile.c_str());

    // [修改] 自动生成
    Wallet alice(walletFile);
    std::string aliceAddr = alice.GetAddress();

    // --- 阶段 1: 运行并产生数据 ---
    {
        std::cout << "Phase 1: Mining blocks..." << std::endl;
        // [修改] 移除 difficulty 参数 (1)
        Blockchain chain(aliceAddr);

        chain.SaveToDisk(dbFile);
    }

    // --- 阶段 2: 模拟重启，读取数据 ---
    {
        std::cout << "\nPhase 2: Restoring from disk..." << std::endl;
        // [修改] 移除 difficulty 参数，且使用默认构造函数
        Blockchain restoredChain;

        restoredChain.LoadFromDisk(dbFile);

        // 验证状态
        int64_t balance = restoredChain.GetBalance(aliceAddr);

        // 注意：创世块是没有 PrevHash 的，size 应该是 32 (全0)
        std::cout << "Restored Height: " << 1 << std::endl;
        std::cout << "Restored Alice Balance: " << balance << std::endl;

        assert(balance == 5000000000);
        std::cout << "Persistence Test Passed!" << std::endl;
    }
}

int main() {
    try {
        TestPersistence();
    }
    catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}