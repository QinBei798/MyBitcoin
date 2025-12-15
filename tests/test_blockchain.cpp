#include "../src/Core/Blockchain.h"
#include "../src/Wallet/Wallet.h"
#include <iostream>
#include <cassert>

// 辅助打印
void PrintResult(bool success, const std::string& msg) {
    if (success) std::cout << "[PASS] " << msg << std::endl;
    else std::cerr << "[FAIL] " << msg << std::endl;
}

void TestDoubleSpend() {
    std::cout << "\n=== Starting Double Spend Test ===\n" << std::endl;

    // 1. 准备 Alice 的钱包
    Wallet alice;
    alice.GenerateNewKey();
    std::string aliceAddr = alice.GetAddress();
    std::cout << "Alice's Address: " << aliceAddr << std::endl;

    // 2. 初始化区块链
    // 关键点：把创世区块的奖励直接给 Alice，这样 Alice 就有 50 BTC 可以花了
    // 难度设为 1，方便秒挖
    Blockchain myChain(1, aliceAddr);
    std::cout << "Blockchain initialized. Genesis reward sent to Alice." << std::endl;

    // 获取创世区块里的那笔交易 ID (作为后续交易的 Input)
    Block genesis = myChain.GetLatestBlock();
    Transaction genesisTx = genesis.transactions[0];
    Bytes genesisTxId = genesisTx.GetId();

    // 检查 Alice 余额
    int64_t balance = myChain.GetBalance(aliceAddr);
    std::cout << "Alice's Balance: " << balance << std::endl;
    assert(balance == 5000000000);

    // ==========================================
    // 场景 A: 正常交易 (Alice -> Bob)
    // ==========================================
    std::cout << "\n--- Scenario A: Alice sends 10 BTC to Bob ---" << std::endl;
    Wallet bob;
    bob.GenerateNewKey();

    // 构造交易 Tx1
    Transaction tx1;
    TxIn in1;
    in1.prevTxId = genesisTxId; // 引用创世交易
    in1.prevIndex = 0;
    in1.publicKey = alice.GetPublicKey();
    tx1.inputs.push_back(in1);

    tx1.outputs.push_back({ 1000000000, bob.GetAddress() }); // 10 BTC 给 Bob
    // 找零 40 BTC 给自己 (简化起见，不设矿工费)
    tx1.outputs.push_back({ 4000000000, alice.GetAddress() });

    // 签名
    tx1.inputs[0].signature = alice.Sign(tx1.GetId());

    // 打包进 Block 1
    Block block1(1, genesis.GetHash(), Bytes(32, 0), 20230101, 1);
    block1.AddTransaction(tx1);
    block1.FinalizeAndMine(1);

    // 尝试上链
    try {
        myChain.AddBlock(block1);
        PrintResult(true, "Block 1 accepted (Normal spend)");
    }
    catch (const std::exception& e) {
        PrintResult(false, std::string("Block 1 rejected: ") + e.what());
        return; // 如果这里失败，后面没法测了
    }

    // 验证余额变化
    // Alice 应该剩 40 BTC (找零)，Bob 有 10 BTC
    assert(myChain.GetBalance(aliceAddr) == 4000000000);
    assert(myChain.GetBalance(bob.GetAddress()) == 1000000000);


    // ==========================================
    // 场景 B: 双花攻击 (Alice -> Carol)
    // Alice 试图再次花费 创世交易 (genesisTxId) 的钱
    // ==========================================
    std::cout << "\n--- Scenario B: Alice tries to Double Spend (Genesis -> Carol) ---" << std::endl;
    Wallet carol;
    carol.GenerateNewKey();

    // 构造交易 Tx2
    // 注意：这里我们故意使用了和 Tx1 一模一样的 Input (genesisTxId, index 0)
    Transaction tx2;
    TxIn in2;
    in2.prevTxId = genesisTxId; // <--- 关键！这个 UTXO 在 Block 1 已经被移除了！
    in2.prevIndex = 0;
    in2.publicKey = alice.GetPublicKey();
    tx2.inputs.push_back(in2);

    tx2.outputs.push_back({ 5000000000, carol.GetAddress() }); // 试图全转给 Carol
    tx2.inputs[0].signature = alice.Sign(tx2.GetId());

    // 打包进 Block 2
    Block latest = myChain.GetLatestBlock();
    Block block2(1, latest.GetHash(), Bytes(32, 0), 20230102, 1);
    block2.AddTransaction(tx2);
    block2.FinalizeAndMine(1);

    // 尝试上链 -> 应该报错！
    try {
        myChain.AddBlock(block2);
        // 如果代码运行到这里，说明 AddBlock 没有抛出异常，双花居然成功了 -> 测试失败
        PrintResult(false, "Block 2 accepted (ERROR: Double spend succeeded!)");
    }
    catch (const std::exception& e) {
        // 捕获异常，说明系统拦截成功 -> 测试通过
        std::cout << "Caught expected error: " << e.what() << std::endl;
        PrintResult(true, "Block 2 rejected (Success: Double spend blocked)");
    }
}

int main() {
    TestDoubleSpend();
    return 0;
}