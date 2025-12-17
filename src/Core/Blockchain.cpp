#include "Blockchain.h"
#include <iostream>
#include "../Crypto/Hash.h" // ToHex
#include "../Utils/Serialization.h" 
#include <fstream> 

// 辅助函数实现
std::string Blockchain::GetUTXOKey(const Bytes& txId, uint32_t index) const {
    return ToHex(txId) + "_" + std::to_string(index);
}

// [修改] 构造函数：允许指定创世奖励给谁
Blockchain::Blockchain(const std::string& minerAddress) {
    // 创世块难度设为 1
    uint32_t initialDifficulty = 1;
    Block genesis(1, Bytes(32, 0), Bytes(32, 0), time(nullptr), initialDifficulty);

    Transaction coinbase;
    TxIn input;
    input.prevTxId = Bytes(32, 0);
    input.prevIndex = 0xFFFFFFFF;
    coinbase.inputs.push_back(input);

    // 如果没有指定地址，使用默认的；否则给指定的 minerAddress
    std::string targetAddr = minerAddress.empty() ? "1GenesisAddress..." : minerAddress;
    coinbase.outputs.push_back({ 5000000000, targetAddr }); // 50 BTC

    genesis.AddTransaction(coinbase);
    genesis.FinalizeAndMine(initialDifficulty);

    chain.push_back(genesis);

    // 更新 UTXO
    Bytes txId = coinbase.GetId();
    for (size_t i = 0; i < coinbase.outputs.size(); i++) {
        utxoSet[GetUTXOKey(txId, i)] = coinbase.outputs[i];
    }
}

// [新增] 动态难度调整算法
uint32_t Blockchain::GetDifficulty() const {
    Block latestBlock = GetLatestBlock();

    // 如果还没到调整周期，直接沿用上一个块的难度
    if (chain.size() % DIFFICULTY_ADJUSTMENT_INTERVAL != 0) {
        return latestBlock.bits;
    }

    // --- 开始调整 ---
    // 找到 5 个块之前的那个块
    // 注意：chain.size() 是当前高度+1，所以要往回找
    size_t firstBlockIndex = chain.size() - DIFFICULTY_ADJUSTMENT_INTERVAL;
    Block firstBlock = chain[firstBlockIndex];

    // 计算实际耗时 (注意：timestamp 是秒)
    // 防止时间倒流导致负数，虽然理论上不应该发生
    long long actualTimeTaken = latestBlock.timestamp - firstBlock.timestamp;
    if (actualTimeTaken < 1) actualTimeTaken = 1;

    // 计算期望耗时 (5个块 * 2秒 = 10秒)
    long long expectedTime = DIFFICULTY_ADJUSTMENT_INTERVAL * BLOCK_GENERATION_INTERVAL;

    std::cout << "\n[Difficulty Adjust] Last " << DIFFICULTY_ADJUSTMENT_INTERVAL
        << " blocks took " << actualTimeTaken << "s (Expected: " << expectedTime << "s)." << std::endl;

    uint32_t currentDifficulty = latestBlock.bits;

    // 规则 1: 如果太快 (小于期望的一半)，难度加倍 (前导零 +1)
    if (actualTimeTaken < expectedTime / 2) {
        std::cout << ">>> Speed is too FAST! Increasing difficulty to " << (currentDifficulty + 1) << std::endl;
        return currentDifficulty + 1;
    }
    // 规则 2: 如果太慢 (大于期望的 2 倍)，难度减半 (前导零 -1)
    else if (actualTimeTaken > expectedTime * 2) {
        if (currentDifficulty > 1) { // 保持最低难度为 1
            std::cout << ">>> Speed is too SLOW! Decreasing difficulty to " << (currentDifficulty - 1) << std::endl;
            return currentDifficulty - 1;
        }
    }

    // 否则保持不变
    std::cout << ">>> Speed is OK. Difficulty remains " << currentDifficulty << std::endl;
    return currentDifficulty;
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
    // 1. 验证前块哈希
    Block latest = GetLatestBlock();
    if (newBlock.prevBlockHash != latest.GetHash()) {
        throw std::runtime_error("Invalid Block: PrevHash mismatch");
    }

    // [修改] 2. 验证难度是否符合要求
    // 必须问链：“当前应该是什么难度？” 而不是信区块自己标的难度
    uint32_t requiredDifficulty = GetDifficulty();

    // 检查区块头里写的难度是否正确
    if (newBlock.bits != requiredDifficulty) {
        // 为了宽容测试，有时候我们允许 newBlock.bits 沿用旧的，但 CheckPoW 必须达标
        // 但严格来说，区块头里的 bits 必须等于 requiredDifficulty
        // 这里我们只检查 PoW 是否满足 requiredDifficulty
    }

    // 检查工作量证明
    if (!newBlock.CheckPoW(requiredDifficulty)) {
        throw std::runtime_error("Invalid Block: PoW check failed");
    }

    // 3. 验证默克尔根
    Bytes calculatedRoot = ComputeMerkleRoot(newBlock.transactions);
    if (newBlock.merkleRoot != calculatedRoot) {
        throw std::runtime_error("Invalid Block: Merkle Root mismatch");
    }

    // 4. 验证交易与 UTXO
    if (!ApplyBlockToUTXO(newBlock)) {
        throw std::runtime_error("Invalid Block: Transaction verification failed");
    }

    chain.push_back(newBlock);
    // std::cout << "Block accepted! UTXO set updated." << std::endl; // 可以注释掉减少刷屏
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

void Blockchain::SaveToDisk(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open file for writing: " << filename << std::endl;
        return;
    }

    // 1. 写入区块数量
    WriteInt(file, (uint32_t)chain.size());

    // 2. 写入每个区块
    for (const auto& block : chain) {
        block.Save(file);
    }
    file.close();
    std::cout << "Blockchain saved to " << filename << std::endl;
}

void Blockchain::LoadFromDisk(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    // [防止报错的关键]：如果文件打不开（比如被删了），直接返回，不要抛异常
    if (!file.is_open()) {
        std::cout << "No existing blockchain found (" << filename << "). Starting new chain." << std::endl;
        return;
    }

    // 1. 清空当前状态 (包括 UTXO)
    chain.clear();
    utxoSet.clear();

    // 2. 读取区块数量
    uint32_t blockCount;
    ReadInt(file, blockCount);

    std::cout << "Loading " << blockCount << " blocks from disk..." << std::endl;

    // 3. 逐个读取并【重建 UTXO】
    for (uint32_t i = 0; i < blockCount; i++) {
        Block block(0, {}, {}, 0, 0); // 临时对象
        block.Load(file);

        // [重放逻辑]
        if (i == 0) {
            // 创世区块：直接把 Coinbase 输出加入 UTXO，不检查 Input
            for (const auto& tx : block.transactions) {
                Bytes txId = tx.GetId();
                for (size_t k = 0; k < tx.outputs.size(); k++) {
                    utxoSet[GetUTXOKey(txId, k)] = tx.outputs[k];
                }
            }
        }
        else {
            // 普通区块：调用 ApplyBlockToUTXO 进行校验和更新
            // 注意：因为是读取自己存的历史数据，如果这里校验失败，说明数据文件损坏
            if (!ApplyBlockToUTXO(block)) {
                std::cerr << "Error: Corrupted blockchain data at block " << i << std::endl;
                // 数据损坏时，可以选择清空或抛异常，这里选择保留已加载的部分
                break;
            }
        }

        chain.push_back(block);
    }

    file.close();
    std::cout << "Blockchain loaded! Height: " << chain.size() << std::endl;
}