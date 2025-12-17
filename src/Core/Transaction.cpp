#include "Transaction.h"
#include "../Utils/Serialization.h"
#include <cstring>

// 辅助工具：写入整数
void PushUInt32(Bytes& data, uint32_t v) {
    data.push_back(v & 0xFF); data.push_back((v >> 8) & 0xFF);
    data.push_back((v >> 16) & 0xFF); data.push_back((v >> 24) & 0xFF);
}
void PushInt64(Bytes& data, int64_t v) {
    for (int i = 0; i < 8; i++) data.push_back((v >> (i * 8)) & 0xFF);
}

// [新增] 写入变长字节数组 (先写长度，再写数据)
void PushBytes(Bytes& data, const Bytes& val) {
    PushUInt32(data, (uint32_t)val.size());          // 1. 写入长度
    data.insert(data.end(), val.begin(), val.end()); // 2. 写入内容
}

// [修改] 序列化：增加 includeSignature 参数
Bytes Transaction::Serialize(bool includeSignature) const {
    Bytes data;

    // 1. Inputs
    PushUInt32(data, (uint32_t)inputs.size());
    for (const auto& in : inputs) {
        // PrevTxId (固定 32 字节，不需要写长度，直接拼接)
        data.insert(data.end(), in.prevTxId.begin(), in.prevTxId.end());

        // PrevIndex
        PushUInt32(data, in.prevIndex);

        // [核心修正] 解决“鸡生蛋”问题
        // 如果 includeSignature 为 false (计算ID时)，写入长度 0，不写签名内容
        if (includeSignature) {
            PushBytes(data, in.signature);
        }
        else {
            PushUInt32(data, 0);
        }

        // 公钥 (通常视为交易意图的一部分，计算 ID 时也要包含)
        PushBytes(data, in.publicKey);
    }

    // 2. Outputs
    PushUInt32(data, (uint32_t)outputs.size());
    for (const auto& out : outputs) {
        PushInt64(data, out.value);

        // 将地址作为锁定脚本 (ScriptPubKey)
        Bytes addrBytes = ToBytes(out.address);
        PushBytes(data, addrBytes); // 使用 PushBytes 自动加上长度前缀
    }

    // 3. LockTime
    PushUInt32(data, lockTime);

    return data;
}

// [修改] GetId: 永远使用“不含签名”的版本计算 ID
Bytes Transaction::GetId() const {
    return Hash256(Serialize(false));
}

// Save (存盘逻辑，包含所有信息)
void Transaction::Save(std::ostream& os) const {
    // 1. Inputs
    WriteInt(os, (uint32_t)inputs.size());
    for (const auto& in : inputs) {
        WriteBytes(os, in.prevTxId);
        WriteInt(os, in.prevIndex);
        WriteBytes(os, in.signature);
        WriteBytes(os, in.publicKey);
    }
    // 2. Outputs
    WriteInt(os, (uint32_t)outputs.size());
    for (const auto& out : outputs) {
        WriteInt(os, out.value);
        WriteString(os, out.address);
    }
    // 3. LockTime
    WriteInt(os, lockTime);
}

// Load (读盘逻辑)
void Transaction::Load(std::istream& is) {
    inputs.clear();
    outputs.clear();

    uint32_t inCount;
    ReadInt(is, inCount);
    for (uint32_t i = 0; i < inCount; i++) {
        TxIn in;
        ReadBytes(is, in.prevTxId);
        ReadInt(is, in.prevIndex);
        ReadBytes(is, in.signature);
        ReadBytes(is, in.publicKey);
        inputs.push_back(in);
    }

    uint32_t outCount;
    ReadInt(is, outCount);
    for (uint32_t i = 0; i < outCount; i++) {
        TxOut out;
        ReadInt(is, out.value);
        ReadString(is, out.address);
        outputs.push_back(out);
    }

    ReadInt(is, lockTime);
}