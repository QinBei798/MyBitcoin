#ifndef BITCOIN_CORE_TRANSACTION_H
#define BITCOIN_CORE_TRANSACTION_H

#include <vector>
#include <string>
#include <cstdint>
#include "../Crypto/Hash.h"

// 交易输入: 引用上一笔钱
struct TxIn {
    Bytes prevTxId; // 上一笔交易的 Hash (32字节)
    uint32_t prevIndex;   // 上一笔交易的第几个输出 (0, 1, ...)
    Bytes signature;      // 解锁脚本(ScriptSig): 这里简化，只存签名
    Bytes publicKey;      // 公钥
};

// 交易输出: 定义这笔钱给谁
struct TxOut {
    int64_t value;        // 金额 (单位: Satoshi)
    std::string address;  // 锁定脚本(ScriptPubKey): 这里简化，直接存对方地址
};

class Transaction {
public:
    std::vector<TxIn> inputs;
    std::vector<TxOut> outputs;
    uint32_t lockTime = 0;

    // 序列化 (用于传输和计算Hash)
    Bytes Serialize() const;

    // 计算交易 ID (即 Hash256(Serialize))
    Bytes GetId() const;
    // [新增] 序列化到文件流
    void Save(std::ostream& os) const;
    void Load(std::istream& is);
};

#endif //BITCOIN_CORE_TRANSACTION_H