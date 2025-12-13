#ifndef BITCOIN_CORE_BLOCKCHAIN_H
#define BITCOIN_CORE_BLOCKCHAIN_H

#include "Block.h"
#include <vector>

class Blockchain {
private:
    std::vector<Block> chain;
    uint32_t difficulty; // 全局难度 (简化版)

public:
    Blockchain(uint32_t diff);

    // 获取最新区块 (用于挖下一个块时引用)
    const Block& GetLatestBlock() const;

    // 添加新区块 (核心验证逻辑)
    void AddBlock(Block newBlock);

    // 打印链状态
    void PrintChain();
};

#endif //BITCOIN_CORE_BLOCKCHAIN_H