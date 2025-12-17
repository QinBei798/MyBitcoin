#include "../src/Wallet/Wallet.h"
#include "../src/Core/Blockchain.h"
#include <iostream>
#include <cassert>
#include <cstdio> // for remove
#include <openssl/applink.c>

void TestWalletTransaction() {
    std::string walletFile = "test_alice_tx.dat";
    std::string chainFile = "test_chain_tx.dat";

    // 清理环境
    remove(walletFile.c_str());
    remove(chainFile.c_str());

    std::cout << "=== Testing Wallet Transaction Creation (with Change) ===" << std::endl;

    // 1. 准备 Alice 的钱包
    Wallet alice(walletFile);
    std::string aliceAddr = alice.GetAddress();
    std::cout << "Alice: " << aliceAddr << std::endl;

    // 2. 准备区块链 (Alice 挖到创世块 -> 获得 50 BTC)
    // 注意：构造函数内部会生成创世块并给 aliceAddr 发 50 BTC
    Blockchain chain(aliceAddr);

    // 验证一下 Alice 确实有钱
    int64_t balance = chain.GetBalance(aliceAddr);
    std::cout << "Alice Initial Balance: " << balance << std::endl;
    assert(balance == 5000000000); // 50 BTC

    // 3. 准备 Bob 的地址 (随便生成一个)
    Wallet bob("test_bob_dummy.dat");
    std::string bobAddr = bob.GetAddress();
    std::cout << "Bob:   " << bobAddr << std::endl;

    // ==========================================
    // 核心测试：创建交易 (Alice -> Bob 10 BTC)
    // ==========================================
    std::cout << "\n[Action] Alice sending 10 BTC to Bob..." << std::endl;

    int64_t sendAmount = 1000000000; // 10 BTC

    // 调用我们刚写的 CreateTransaction
    Transaction tx = alice.CreateTransaction(bobAddr, sendAmount, chain);

    // ==========================================
    // 验证结果
    // ==========================================

    // 1. 检查输入 (Inputs)
    // 因为 Alice 只有一笔 50 BTC 的钱，所以应该只有 1 个 Input
    std::cout << "Inputs count: " << tx.inputs.size() << std::endl;
    assert(tx.inputs.size() == 1);
    // 这里的 Input 应该引用了创世块的 Coinbase 交易

    // 2. 检查输出 (Outputs)
    // 应该有 2 个 Output: 
    // - 一个给 Bob (10 BTC)
    // - 一个找零给 Alice (40 BTC)
    std::cout << "Outputs count: " << tx.outputs.size() << std::endl;
    assert(tx.outputs.size() == 2);

    // 3. 验证金额和地址
    bool foundRecipient = false;
    bool foundChange = false;

    for (const auto& out : tx.outputs) {
        if (out.address == bobAddr) {
            std::cout << "  -> Output to Bob: " << out.value << " (Expected: 10 BTC)" << std::endl;
            assert(out.value == 1000000000);
            foundRecipient = true;
        }
        else if (out.address == aliceAddr) {
            std::cout << "  -> Output to Alice (Change): " << out.value << " (Expected: 40 BTC)" << std::endl;
            assert(out.value == 4000000000); // 50 - 10 = 40
            foundChange = true;
        }
    }

    if (foundRecipient && foundChange) {
        std::cout << "[PASS] Transaction structure is correct!" << std::endl;
    }
    else {
        std::cerr << "[FAIL] Missing recipient or change output!" << std::endl;
        exit(1);
    }

    // 4. 验证签名是否存在
    // CreateTransaction 应该自动为 inputs 签名
    if (!tx.inputs[0].signature.empty()) {
        std::cout << "[PASS] Input is signed." << std::endl;
    }
    else {
        std::cerr << "[FAIL] Input is NOT signed!" << std::endl;
        exit(1);
    }
}

int main() {
    try {
        TestWalletTransaction();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}