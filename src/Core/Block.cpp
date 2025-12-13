#include "Block.h"
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
    // 检查哈希值是小端序还是大端序？
    // 在比特币内部计算通常用 Little Endian，但展示给人类看通常反转成 Big Endian。
    // 这里我们简单起见，假设 hash 数组的最后一个字节是最高位（因为 Hash256 结果通常被视为大数）。
    // 实际上，只要我们约定好即可。
    // 让我们用最直观的方式：检查 hash 数组 **最后面** 的字节是否为 0 (因为比特币内部显示通常反转)。
    // 为了通过下面的测试，我们简单检查哈希数组 **反转后** 的前导零个数。

    Bytes hashReverse = hash;
    std::reverse(hashReverse.begin(), hashReverse.end());

    for (uint32_t i = 0; i < difficulty_zeros; i++) {
        if (hashReverse[i] != 0) return false;
    }
    return true;
}

void Block::Mine(uint32_t difficulty_zeros) {
    std::cout << "Mining started... Target: " << difficulty_zeros << " leading zeros." << std::endl;
    nonce = 0;
    while (true) {
        if (CheckPoW(difficulty_zeros)) {
            std::cout << "Block Mined! Nonce: " << nonce << std::endl;
            std::cout << "Hash: " << ToHex(GetHash()) << std::endl;
            break;
        }
        nonce++;

        // 防止溢出 (实际不太可能在测试中溢出)
        if (nonce == 0) {
            std::cout << "Nonce overflow, updating timestamp..." << std::endl;
            timestamp++;
        }
    }
}