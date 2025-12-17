#include "../src/Core/Blockchain.h"
#include "../src/Wallet/Wallet.h"
#include <iostream>
#include <cassert>
#include <cstdio> // for remove

void TestPersistence() {
    std::string dbFile = "blockchain.dat";

    // 为了演示“数据消失程序不出错”，我们先手动删一下文件
    remove(dbFile.c_str());

    Wallet alice; // 注意：如果你还没有实现 Wallet 持久化，这里每次都是新 Alice
    // 建议配合 Wallet 持久化一起测试，或者像下面这样只测区块链

    std::string aliceAddr = alice.GetAddress();
    std::cout << "Alice Address: " << aliceAddr << std::endl;

    // --- 阶段 1: 产生数据 ---
    {
        std::cout << "\n[Phase 1] Mining..." << std::endl;
        Blockchain chain(1, aliceAddr); // 创世块给 Alice

        // 此时 Alice 有 50 BTC
        // 我们保存并退出
        chain.SaveToDisk(dbFile);
    }

    // --- 阶段 2: 恢复数据 ---
    {
        std::cout << "\n[Phase 2] Restoring..." << std::endl;
        // 创建新链对象
        Blockchain restoredChain(1);

        // 加载
        restoredChain.LoadFromDisk(dbFile);

        // 验证 UTXO 是否重建成功
        int64_t balance = restoredChain.GetBalance(aliceAddr);
        std::cout << "Restored Alice Balance: " << balance << std::endl;

        if (balance == 5000000000) {
            std::cout << "[PASS] Persistence successful!" << std::endl;
        }
        else {
            std::cerr << "[FAIL] UTXO rebuild failed!" << std::endl;
        }
    }
}

int main() {
    TestPersistence();
    return 0;
}