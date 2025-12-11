#include "Hash.h"
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <iomanip>
#include <sstream>

// 1. 实现 SHA-256
Bytes Sha256(const Bytes& data) {
    Bytes hash(SHA256_DIGEST_LENGTH); // 32 字节
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, data.data(), data.size());
    SHA256_Final(hash.data(), &sha256);
    return hash;
}

// 2. 实现 Hash256 (Double SHA-256)
Bytes Hash256(const Bytes& data) {
    return Sha256(Sha256(data));
}

// 3. 实现 RIPEMD-160
Bytes Ripemd160(const Bytes& data) {
    Bytes hash(RIPEMD160_DIGEST_LENGTH); // 20 字节
    RIPEMD160_CTX ripemd;
    RIPEMD160_Init(&ripemd);
    RIPEMD160_Update(&ripemd, data.data(), data.size());
    RIPEMD160_Final(hash.data(), &ripemd);
    return hash;
}

// 4. 实现 Hash160 (SHA256 + RIPEMD160)
Bytes Hash160(const Bytes& data) {
    return Ripemd160(Sha256(data));
}

// 5. Hex 工具实现
std::string ToHex(const Bytes& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : data) {
        ss << std::setw(2) << (int)byte;
    }
    return ss.str();
}

Bytes ToBytes(const std::string& str) {
    Bytes data(str.begin(), str.end());
    return data;
}