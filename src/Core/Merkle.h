#ifndef BITCOIN_CORE_MERKLE_H
#define BITCOIN_CORE_MERKLE_H

#include "../Crypto/Hash.h"
#include "Transaction.h"
#include <vector>

// 计算默克尔树根
// 输入：所有交易的列表
// 输出：32字节的根哈希
Bytes ComputeMerkleRoot(const std::vector<Transaction>& txs);

#endif //BITCOIN_CORE_MERKLE_H