#include "Blockchain.h"
#include <iostream>
#include "../Crypto/Hash.h" // ToHex

Blockchain::Blockchain(uint32_t diff) : difficulty(diff) {
    // 1. 创建创世区块 (Genesis Block)
    // 前块哈希全0，默克尔根全0 (简化)
    Block genesis(1, Bytes(32, 0), Bytes(32, 0), 12345, difficulty);
    genesis.Mine(difficulty);
    chain.push_back(genesis);
}

const Block& Blockchain::GetLatestBlock() const {
    return chain.back();
}

void Blockchain::AddBlock(Block newBlock) {
    // --- 全节点验证流程 ---

    // 1. 验证前块哈希 (链接是否断裂)
    Block latest = GetLatestBlock();
    if (newBlock.prevBlockHash != latest.GetHash()) {
        throw std::runtime_error("Invalid Block: PrevHash mismatch");
    }

    // 2. 验证 PoW (工作量证明是否达标)
    if (!newBlock.CheckPoW(difficulty)) {
        throw std::runtime_error("Invalid Block: PoW check failed");
    }

    // 3. 验证默克尔根 (交易数据是否被篡改)
    Bytes calculatedRoot = ComputeMerkleRoot(newBlock.transactions);
    if (newBlock.merkleRoot != calculatedRoot) {
        throw std::runtime_error("Invalid Block: Merkle Root mismatch");
    }

    // 4. (可选) 验证每笔交易的签名 ...

    // 全部通过，上链
    chain.push_back(newBlock);
    std::cout << "Block accepted! Height: " << chain.size() << std::endl;
}

void Blockchain::PrintChain() {
    for (size_t i = 0; i < chain.size(); i++) {
        std::cout << "Height: " << i
            << " | Hash: " << ToHex(chain[i].GetHash())
            << " | TxCount: " << chain[i].transactions.size() << std::endl;
    }
}