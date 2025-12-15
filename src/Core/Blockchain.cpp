#include "Blockchain.h"
#include <iostream>
#include "../Crypto/Hash.h" // ToHex

// 辅助函数实现
std::string Blockchain::GetUTXOKey(const Bytes& txId, uint32_t index) const {
    return ToHex(txId) + "_" + std::to_string(index);
}

// [修改] 构造函数：允许指定创世奖励给谁
Blockchain::Blockchain(uint32_t diff, const std::string& minerAddress) : difficulty(diff) {
    Block genesis(1, Bytes(32, 0), Bytes(32, 0), 12345, difficulty);

    Transaction coinbase;
    TxIn input;
    input.prevTxId = Bytes(32, 0);
    input.prevIndex = 0xFFFFFFFF;
    coinbase.inputs.push_back(input);

    // 如果没有指定地址，使用默认的；否则给指定的 minerAddress
    std::string targetAddr = minerAddress.empty() ? "1GenesisAddress..." : minerAddress;
    coinbase.outputs.push_back({ 5000000000, targetAddr }); // 50 BTC

    genesis.AddTransaction(coinbase);
    genesis.FinalizeAndMine(difficulty);

    chain.push_back(genesis);

    // 更新 UTXO
    Bytes txId = coinbase.GetId();
    for (size_t i = 0; i < coinbase.outputs.size(); i++) {
        utxoSet[GetUTXOKey(txId, i)] = coinbase.outputs[i];
    }
}

// 核心：处理区块并更新 UTXO
// 核心：处理区块并更新 UTXO (原子性操作版本)
bool Blockchain::ApplyBlockToUTXO(const Block& block) {
    // [步骤 1] 创建影子副本 (Deep Copy)
    // 我们所有的操作都在这个 tempUTXO 上进行，绝对不碰真正的 utxoSet
    // 只有当一切验证都通过时，我们才会在函数末尾更新 utxoSet
    std::map<UTXOKey, TxOut> tempUTXO = utxoSet;

    // [步骤 2] 遍历区块中的每一笔交易
    for (const auto& tx : block.transactions) {
        Bytes txId = tx.GetId();

        // 判断是否为 Coinbase 交易 (挖矿奖励，无输入)
        // 简单判断：如果 inputs[0].prevIndex == 0xFFFFFFFF 则是 Coinbase
        bool isCoinbase = (tx.inputs.size() == 1 && tx.inputs[0].prevIndex == 0xFFFFFFFF);

        if (!isCoinbase) {
            // --- 验证输入 (花钱) ---
            int64_t inputSum = 0;
            int64_t outputSum = 0;

            for (const auto& in : tx.inputs) {
                std::string key = GetUTXOKey(in.prevTxId, in.prevIndex);

                // 检查 1: 钱是否存在？
                // 注意：我们在 tempUTXO 中查找。这允许一个区块内的“链式交易”
                // (即 Tx B 花费了同区块内 Tx A 刚刚生成的 Output)
                if (tempUTXO.find(key) == tempUTXO.end()) {
                    std::cerr << "Error: UTXO missing or double spent! Key: " << key << std::endl;
                    return false; // 失败直接返回，utxoSet 毫发无损
                }

                // 获取引用的 Output
                const TxOut& prevOut = tempUTXO[key];

                // 检查 2: 签名验证 (简略版，调用 Wallet::Verify)
                // 在完整实现中，这里需要根据 prevOut.address 解析公钥进行验证
                // if (!Wallet::Verify(..., in.signature)) return false;

                inputSum += prevOut.value;
            }

            // 计算输出总额
            for (const auto& out : tx.outputs) {
                outputSum += out.value;
            }

            // 检查 3: 余额是否足够？(输入 >= 输出)
            if (inputSum < outputSum) {
                std::cerr << "Error: Insufficient funds! In: " << inputSum << " Out: " << outputSum << std::endl;
                return false; // 失败直接返回
            }
        }

        // --- 更新影子账本 (tempUTXO) ---

        // 1. 移除已花费的 Input (销毁钱)
        if (!isCoinbase) {
            for (const auto& in : tx.inputs) {
                std::string key = GetUTXOKey(in.prevTxId, in.prevIndex);
                tempUTXO.erase(key); // 从影子账本中抹除
            }
        }

        // 2. 添加新的 Output (生成钱)
        for (size_t i = 0; i < tx.outputs.size(); i++) {
            std::string key = GetUTXOKey(txId, i);
            tempUTXO[key] = tx.outputs[i]; // 写入影子账本
        }
    }

    // [步骤 3] 提交事务 (Commit)
    // 只有代码运行到这里，说明所有交易都合法。
    // 现在我们将影子副本覆盖回主数据。
    utxoSet = tempUTXO;

    return true;
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

    std::map<UTXOKey, TxOut> backupUTXO = utxoSet; // 备份

    // 2. 交易逻辑验证 & UTXO 原子更新
     // ApplyBlockToUTXO 内部使用了副本机制，如果失败返回 false，utxoSet 不变
    if (!ApplyBlockToUTXO(newBlock)) {
        throw std::runtime_error("Invalid Block: Transaction verification failed");
    }

    // 3. 上链
    chain.push_back(newBlock);
    std::cout << "Block accepted! UTXO set updated." << std::endl;
}

void Blockchain::PrintChain() {
    for (size_t i = 0; i < chain.size(); i++) {
        std::cout << "Height: " << i
            << " | Hash: " << ToHex(chain[i].GetHash())
            << " | TxCount: " << chain[i].transactions.size() << std::endl;
    }
}

int64_t Blockchain::GetBalance(const std::string& address) const {
    int64_t total = 0;
    for (const auto& pair : utxoSet) {
        if (pair.second.address == address) {
            total += pair.second.value;
        }
    }
    return total;
}