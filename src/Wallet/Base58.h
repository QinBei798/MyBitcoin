#ifndef BITCOIN_WALLET_BASE58_H
#define BITCOIN_WALLET_BASE58_H

#include <string>
#include <vector>
#include "../Crypto/Hash.h" // 需要用到 bytes 类型

// 基础 Base58 编码
std::string EncodeBase58(const Bytes& data);

// Base58Check 编码 (Base58 + 4字节校验和)
// 比特币地址就是用这个生成的
std::string EncodeBase58Check(const Bytes& data);

#endif //BITCOIN_WALLET_BASE58_H