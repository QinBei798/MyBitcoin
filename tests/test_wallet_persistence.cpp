#include "../src/Wallet/Wallet.h"
#include "../src/Core/Blockchain.h"
#include <iostream>
#include <cassert>
#include <openssl/applink.c>

// 这个测试会运行两次来验证持久化
// 第一次：生成 wallet.dat，挖矿获得 50 BTC，保存区块链
// 第二次：读取 wallet.dat，读取区块链，确认余额还是 50 BTC

void TestWalletPersistence() {
    std::string walletFile = "my_wallet.dat";
    std::string chainFile = "my_blockchain.dat";

    // --- 第一步：初始化并挖矿 ---
    {
        std::cout << "--- Session 1: Creating Wallet & Mining ---" << std::endl;
        Wallet myWallet(walletFile); // 自动生成并保存
        std::cout << "My Address: " << myWallet.GetAddress() << std::endl;

        // 初始化区块链，把创世奖励给我
        Blockchain chain(1, myWallet.GetAddress());

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

        // 1. 加载钱包 (应该自动从 my_wallet.dat 读取，而不是生成新的)
        Wallet oldWallet(walletFile);
        std::cout << "Loaded Address: " << oldWallet.GetAddress() << std::endl;

        // 2. 加载区块链
        Blockchain restoredChain(1);
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
    // 为了测试干净，先删除旧文件 (如果有的话)
    remove("my_wallet.dat");
    remove("my_blockchain.dat");

    TestWalletPersistence();
    return 0;
}