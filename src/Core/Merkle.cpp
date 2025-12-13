#include "Merkle.h"

Bytes ComputeMerkleRoot(const std::vector<Transaction>& txs) {
    if (txs.empty()) return Bytes(32, 0);

    // 1. 提取所有交易的 TxID (Hash)
    std::vector<Bytes> hashes;
    for (const auto& tx : txs) {
        hashes.push_back(tx.GetId());
    }

    // 2. 层层向上计算
    while (hashes.size() > 1) {
        // 如果是奇数个，复制最后一个补齐
        if (hashes.size() % 2 != 0) {
            hashes.push_back(hashes.back());
        }

        std::vector<Bytes> newLevel;
        // 两两配对
        for (size_t i = 0; i < hashes.size(); i += 2) {
            // 拼接 left + right
            Bytes concat = hashes[i];
            concat.insert(concat.end(), hashes[i + 1].begin(), hashes[i + 1].end());
            // 计算双重哈希
            newLevel.push_back(Hash256(concat));
        }
        hashes = newLevel;
    }

    return hashes[0];
}