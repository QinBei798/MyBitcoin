#include "../src/Core/Blockchain.h"
#include <iostream>
#include <thread>
#include <vector>

// 辅助：快速挖矿并上链
void MineBlock(Blockchain& chain, uint32_t& simulatedTime) {
    Block prev = chain.GetLatestBlock();

    // 1. 获取当前应该的难度
    uint32_t difficulty = chain.GetDifficulty();

    // 2. 伪造时间戳：假设我们是超级计算机，每 0 秒就挖出一个块（非常快）
    // 这样应该会触发“难度提升”
    simulatedTime += 0; // 这里的 0 表示瞬间挖出

    Block newBlock(1, prev.GetHash(), Bytes(32, 0), simulatedTime, difficulty);

    // 加个空交易防止 Merkle 报错
    Transaction tx;
    tx.inputs.push_back({ Bytes(32,0), 0xFFFFFFFF, {}, {} }); // Coinbase
    newBlock.AddTransaction(tx);

    // 挖矿
    newBlock.FinalizeAndMine(difficulty);

    // 上链
    chain.AddBlock(newBlock);
}

void TestDynamicDifficulty() {
    std::cout << "=== Testing Dynamic Difficulty Adjustment ===" << std::endl;

    Blockchain chain("miner_addr");

    // 初始时间 (用创世块的时间)
    uint32_t simulatedTime = chain.GetLatestBlock().timestamp;

    std::cout << "Initial Difficulty: " << chain.GetDifficulty() << std::endl;

    // 我们设定每 5 个块调整一次。
    // 我们挖 15 个块，看看难度会不会阶梯式上升。

    for (int i = 1; i <= 15; i++) {
        MineBlock(chain, simulatedTime);
        std::cout << "Block " << i << " mined. Current Height: " << i + 1 << std::endl;

        // 每次是 5 的倍数时，打印一下新计算出的难度
        if ((i + 1) % 5 == 0) {
            std::cout << "--- Checkpoint: Next Block Difficulty will be: " << chain.GetDifficulty() << " ---" << std::endl;
        }
    }
}

int main() {
    TestDynamicDifficulty();
    return 0;
}