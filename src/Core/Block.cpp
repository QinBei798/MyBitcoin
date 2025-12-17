#include "Block.h"
#include "../Utils/Serialization.h"
#include <iostream>
#include <cstring>
#include <algorithm> // for std::reverse if needed

// 辅助：将整数转为小端序字节数组
void AppendUInt32(Bytes& data, uint32_t value) {
    data.push_back(value & 0xFF);
    data.push_back((value >> 8) & 0xFF);
    data.push_back((value >> 16) & 0xFF);
    data.push_back((value >> 24) & 0xFF);
}

Block::Block(int32_t ver, const Bytes& prev, const Bytes& root, uint32_t time, uint32_t difficulty_bits)
    : version(ver), prevBlockHash(prev), merkleRoot(root), timestamp(time), bits(difficulty_bits), nonce(0) {
}

void Block::AddTransaction(const Transaction& tx) {
    transactions.push_back(tx);
}

void Block::FinalizeAndMine(uint32_t difficulty_zeros) {
    // 1. 在挖矿前，根据当前的交易列表计算 Merkle Root 并填入区块头
    if (!transactions.empty()) {
        merkleRoot = ComputeMerkleRoot(transactions);
    }

    // 2. 调用之前的挖矿逻辑
    Mine(difficulty_zeros);
}

Bytes Block::Serialize() const {
    Bytes data;
    // 必须严格按照比特币协议顺序拼接这 6 个字段
    AppendUInt32(data, version);                    // 4 Bytes
    data.insert(data.end(), prevBlockHash.begin(), prevBlockHash.end()); // 32 Bytes
    data.insert(data.end(), merkleRoot.begin(), merkleRoot.end());       // 32 Bytes
    AppendUInt32(data, timestamp);                  // 4 Bytes
    AppendUInt32(data, bits);                       // 4 Bytes
    AppendUInt32(data, nonce);                      // 4 Bytes
    return data;
}

Bytes Block::GetHash() const {
    return Hash256(Serialize());
}

// 简化的难度检查：这里我们暂时不解析复杂的 bits (如 0x1d00ffff)，
// 而是简单地检查哈希值的前 N 位是否为 0。
// 真正的比特币代码在 main.cpp 里用 bignum 比较。
bool Block::CheckPoW(uint32_t difficulty_zeros) const {
    Bytes hash = GetHash();

    // 我们直接检查哈希数组的前 N 个字节
    // 这样 ToHex(hash) 打印出来时，零就在最前面
    for (uint32_t i = 0; i < difficulty_zeros; i++) {
        // 防止数组越界 (虽然难度不太可能达到 32)
        if (i >= hash.size()) return false;

        if (hash[i] != 0) return false;
    }
    return true;
}

// [修改] 挖矿函数
void Block::Mine(uint32_t difficulty_zeros) {
    std::cout << "Mining started... Target: " << difficulty_zeros << " leading zeros (at the beginning)." << std::endl;
    nonce = 0;

    // 获取开始时间，用于统计算力 (可选)
    time_t start = time(nullptr);

    while (true) {
        // 每 100,000 次尝试检查一下是否溢出或输出进度
        if (nonce % 100000 == 0 && nonce != 0) {
            // std::cout << "Nonce: " << nonce << "..." << std::endl; 
        }

        if (CheckPoW(difficulty_zeros)) {
            time_t end = time(nullptr);
            std::cout << "Block Mined!" << std::endl;
            std::cout << "  - Nonce: " << nonce << std::endl;
            std::cout << "  - Hash:  " << ToHex(GetHash()) << std::endl; // 这里打印出来应该是 000... 开头
            std::cout << "  - Time:  " << (end - start) << " seconds" << std::endl;
            break;
        }

        nonce++;

        // 防止 nonce 溢出 (虽然 uint32 能存 40 亿，很难溢出)
        if (nonce == 0) {
            std::cout << "Nonce overflow, updating timestamp..." << std::endl;
            timestamp = time(nullptr);
        }
    }
}

void Block::Save(std::ostream& os) const {
    // Header
    WriteInt(os, version);
    WriteBytes(os, prevBlockHash);
    WriteBytes(os, merkleRoot);
    WriteInt(os, timestamp);
    WriteInt(os, bits);
    WriteInt(os, nonce);

    // Transactions
    WriteInt(os, (uint32_t)transactions.size());
    for (const auto& tx : transactions) {
        tx.Save(os);
    }
}

void Block::Load(std::istream& is) {
    transactions.clear();

    ReadInt(is, version);
    ReadBytes(is, prevBlockHash);
    ReadBytes(is, merkleRoot);
    ReadInt(is, timestamp);
    ReadInt(is, bits);
    ReadInt(is, nonce);

    uint32_t txCount;
    ReadInt(is, txCount);
    for (uint32_t i = 0; i < txCount; i++) {
        Transaction tx;
        tx.Load(is);
        transactions.push_back(tx);
    }
}