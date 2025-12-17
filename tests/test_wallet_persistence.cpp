#include "../src/Wallet/Wallet.h"
#include "../src/Core/Blockchain.h"
#include <iostream>
#include <cassert>
#include <cstdio>
// [关键] 解决 Windows 下 OpenSSL 错误
#include <openssl/applink.c> 

void TestWalletPersistence() {
    std::string walletFile = "my_wallet.dat";
    std::string chainFile = "my_blockchain.dat";

    // 清理旧文件
    remove(walletFile.c_str());
    remove(chainFile.c_str());

    // --- 第一步：初始化并挖矿 ---
    {
        std::cout << "--- Session 1: Creating Wallet & Mining ---" << std::endl;
        Wallet myWallet(walletFile); // 自动生成并保存
        std::cout << "My Address: " << myWallet.GetAddress() << std::endl;

        // [修改] 移除 difficulty 参数
        Blockchain chain(myWallet.GetAddress());

        // 确认现在余额是 50 BTC
        int64_t balance = chain.GetBalance(myWallet.GetAddress());
        std::cout << "Current Balance: " << balance << std::endl;

        // 保存区块链
        chain.SaveToDisk(chainFile);
    }

    std::cout << "\n... Simulating Restart ...\n" << std::endl;

    // --- 第二步：重启并读取 ---
    {
        std::cout << "--- Session 2: Loading Wallet & Checking Balance ---" << std::endl;

        // 1. 加载钱包
        Wallet oldWallet(walletFile);
        std::cout << "Loaded Address: " << oldWallet.GetAddress() << std::endl;

        // 2. 加载区块链
        // [修改] 移除 difficulty 参数，使用默认构造函数
        Blockchain restoredChain;
        restoredChain.LoadFromDisk(chainFile);

        // 3. 验证余额
        int64_t balance = restoredChain.GetBalance(oldWallet.GetAddress());
        std::cout << "Restored Balance: " << balance << std::endl;

        if (balance == 5000000000) {
            std::cout << "[SUCCESS] Wallet and Balance persisted successfully!" << std::endl;
        }
        else {
            std::cerr << "[FAIL] Balance mismatch! Expected 5000000000" << std::endl;
        }
    }
}

int main() {
    TestWalletPersistence();
    return 0;
}