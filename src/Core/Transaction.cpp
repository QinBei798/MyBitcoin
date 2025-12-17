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

Bytes Transaction::Serialize() const {
    Bytes data;
    // 简化的序列化格式:
    // [InCount] [In1] [In2]... [OutCount] [Out1] [Out2]...

    // 1. Inputs
    PushUInt32(data, inputs.size());
    for (const auto& in : inputs) {
        data.insert(data.end(), in.prevTxId.begin(), in.prevTxId.end());
        PushUInt32(data, in.prevIndex);
        // 注意：计算交易ID时，签名部分通常需要特殊处理(置空)，这里为了演示最简逻辑，暂包含进去
        // 在正式比特币中，TxID 的计算不包含 Witness 数据，但包含 ScriptSig
    }

    // 2. Outputs
    PushUInt32(data, outputs.size());
    for (const auto& out : outputs) {
        PushInt64(data, out.value);
        // 简单把地址放进去作为 ScriptPubKey
        Bytes addrBytes = ToBytes(out.address);
        data.insert(data.end(), addrBytes.begin(), addrBytes.end());
    }

    return data;
}

Bytes Transaction::GetId() const {
    return Hash256(Serialize());
}

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