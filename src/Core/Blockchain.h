#ifndef BITCOIN_CORE_BLOCKCHAIN_H
#define BITCOIN_CORE_BLOCKCHAIN_H

#include "Block.h"
#include <vector>
#include <map>
#include <string>
#include "Transaction.h"

using UTXOKey = std::string;

class Blockchain {
private:
    std::vector<Block> chain;
   // uint32_t difficulty; // 全局难度 (简化版) 现改为动态调整

    // [新增] 调整参数
    const uint32_t DIFFICULTY_ADJUSTMENT_INTERVAL = 5; // 每 5 个块调整一次
    const uint32_t BLOCK_GENERATION_INTERVAL = 2;      // 期望 2 秒出一个块

    // [新增] UTXO 集合: 记录全网所有未花费的钱
    // Key: "TxID_OutputIndex", Value: TxOut (包含金额和地址)
    std::map<UTXOKey, TxOut> utxoSet;

    // [新增] 辅助函数: 生成 Key
    std::string GetUTXOKey(const Bytes& txId, uint32_t index) const;

    // [新增] 核心逻辑: 尝试根据区块更新 UTXO (如果失败则回滚)
    // 返回 true 表示区块合法且 UTXO 更新成功
    bool ApplyBlockToUTXO(const Block& block);

public:
    // [修改] 增加 minerAddress 参数，默认为空
    Blockchain(const std::string& minerAddress = "");

    // 获取最新区块 (用于挖下一个块时引用)
    const Block& GetLatestBlock() const;

    // 添加新区块 (核心验证逻辑)
    void AddBlock(Block newBlock);

    // [新增] 查询余额 (遍历 UTXO Set)
    int64_t GetBalance(const std::string& address) const;

    // [新增]
    void SaveToDisk(const std::string& filename) const;
    void LoadFromDisk(const std::string& filename);

    // [新增] 找出属于某个地址的所有 UTXO
    // 返回 map<UTXOKey, TxOut>
    std::map<std::string, TxOut> FindUTXOs(const std::string& address) const;

    // 打印链状态
    void PrintChain();

    // [新增] 核心：获取下一个区块所需的难度
    uint32_t GetDifficulty() const;
};

#endif //BITCOIN_CORE_BLOCKCHAIN_H