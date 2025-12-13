#include "../src/Core/Blockchain.h"
#include "../src/Wallet/Wallet.h"
#include <iostream>

void TestFullFlow() {
    std::cout << "=== Bitcoin System Starting ===" << std::endl;

    // 1. 初始化区块链 (难度设为 2，方便快速挖矿)
    Blockchain myChain(2);

    // 2. 模拟用户
    Wallet alice, bob;
    alice.GenerateNewKey();
    bob.GenerateNewKey();
    std::cout << "Alice Addr: " << alice.GetAddress() << std::endl;

    // 3. 创建一笔交易: Alice -> Bob
    TxIn input;
    input.prevTxId = Bytes(32, 0xFF); // 模拟引用之前的钱
    input.prevIndex = 0;
    input.publicKey = alice.GetPublicKey();

    Transaction tx1;
    tx1.inputs.push_back(input);
    tx1.outputs.push_back({ 100, bob.GetAddress() }); // 转 100 Satoshi

    // 签名 (简略)
    tx1.inputs[0].signature = alice.Sign(tx1.GetId());

    // 4. 矿工打包区块
    std::cout << "\n[Miner] Packing block..." << std::endl;

    // 获取前一个区块的哈希
    Block prevBlock = myChain.GetLatestBlock();

    // 创建新区块
    // 注意：MerkleRoot 暂时填空，稍后 Finalize 会自动计算
    Block newBlock(1, prevBlock.GetHash(), Bytes(32, 0), 20231001, 2);

    // 放入交易
    newBlock.AddTransaction(tx1);

    // 5. 挖矿 (计算 Merkle Root + PoW)
    newBlock.FinalizeAndMine(2);

    // 6. 广播并上链
    std::cout << "\n[Network] Broadcasting block..." << std::endl;
    try {
        myChain.AddBlock(newBlock);
        std::cout << "SUCCESS: Block added to main chain!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "FAILED: " << e.what() << std::endl;
    }

    myChain.PrintChain();
}

int main() {
    TestFullFlow();
    return 0;
}