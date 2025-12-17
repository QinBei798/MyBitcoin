#ifndef BITCOIN_CRYPTO_HASH_H
#define BITCOIN_CRYPTO_HASH_H

#include <vector>
#include <string>
#include <cstdint>

// 定义字节类型，方便阅读
using Bytes = std::vector<uint8_t>;

// 1. 基础 SHA-256
Bytes Sha256(const Bytes& data);

// 2. 双重 SHA-256 (对应 v0.1.5 util.h 中的 Hash 函数)
// 用途：工作量证明(PoW)、区块哈希、交易ID
Bytes Hash256(const Bytes& data);

// 3. 基础 RIPEMD-160
Bytes Ripemd160(const Bytes& data);

// 4. Hash160 (先SHA256后RIPEMD160) (对应 v0.1.5 util.h 中的 Hash160 函数)
// 用途：生成比特币地址
Bytes Hash160(const Bytes& data);

// 5. 辅助工具：将字节转为16进制字符串
std::string ToHex(const Bytes& data);

// 6. 辅助工具：将字符串转为字节流
Bytes ToBytes(const std::string& str);

// [新增] 辅助工具：将16进制字符串转回字节数组 (用于解析 UTXO Key)
inline Bytes FromHex(const std::string& hex) {
    Bytes bytes;
    for (unsigned int i = 0; i < hex.length(); i += 2) {
        std::string byteString = hex.substr(i, 2);
        char byte = (char)strtol(byteString.c_str(), nullptr, 16);
        bytes.push_back(byte);
    }
    return bytes;
}

#endif //BITCOIN_CRYPTO_HASH_H