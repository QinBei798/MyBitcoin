#ifndef BITCOIN_CORE_BLOCK_H
#define BITCOIN_CORE_BLOCK_H

#include <cstdint>
#include <vector>
#include <string>
#include "../Crypto/Hash.h" // 引用昨天的成果
#include "Transaction.h" // 新增
#include "Merkle.h"      // 新增

class Block {
public:
    // --- 1. 区块头结构 (共 80 字节) ---
    // 参考 v0.1.5 main.h 中的 CBlock
    int32_t version;            // 版本号
    Bytes prevBlockHash;        // 前一个区块的哈希 (32字节)
    Bytes merkleRoot;           // 交易树根哈希 (32字节)
    uint32_t timestamp;         // 时间戳
    uint32_t bits;              // 难度目标 (Target)
    uint32_t nonce;             // 随机数 (矿工唯一能改的东西)
    // [新增] 交易列表本体
    std::vector<Transaction> transactions;

    // --- 构造函数 ---
    Block(int32_t ver, const Bytes& prev, const Bytes& root, uint32_t time, uint32_t difficulty_bits);

    // --- 2. 核心功能 ---
    // 
    // [新增] 添加交易
    void AddTransaction(const Transaction& tx);

    // [修改] 挖矿前，先计算 Merkle Root
    void FinalizeAndMine(uint32_t difficulty_zeros);

    // 序列化：将区块头转为字节流 (用于计算哈希)
    Bytes Serialize() const;

    // 计算当前区块的哈希 ID (即 Hash256(Serialize()))
    Bytes GetHash() const;

    // 挖矿函数：不断修改 nonce，直到 GetHash() < Target
    void Mine(uint32_t difficulty_bits);

    // 辅助：检查当前哈希是否满足难度
    bool CheckPoW(uint32_t difficulty_bits) const;
};

#endif //BITCOIN_CORE_BLOCK_H