#ifndef BITCOIN_WALLET_WALLET_H
#define BITCOIN_WALLET_WALLET_H

#include "../Crypto/Hash.h"
#include <openssl/ec.h>
#include <openssl/obj_mac.h> // NID_secp256k1
#include <string>

class Wallet {
private:
    EC_KEY* pKey; // OpenSSL 的 ECC 密钥结构体
    std::string filename; // 钱包文件名

    // 生成新的随机私钥
    void GenerateNewKey();

public:
    // [修改] 构造函数带默认文件名
    explicit Wallet(const std::string& file = "wallet.dat");
    ~Wallet();

    // [新增] 保存与加载
    void Save();
    bool Load();


    // 获取公钥 (字节流形式)
    Bytes GetPublicKey() const;

    // 获取钱包地址 (Base58Check 编码的公钥哈希)
    std::string GetAddress() const;

    // [新增] 使用私钥对数据哈希进行签名 (返回 DER 格式的签名)
    Bytes Sign(const Bytes& hash) const;

    // [新增] 静态函数：验证签名是否有效
    static bool Verify(const Bytes& pubKey, const Bytes& hash, const Bytes& signature);
};

#endif //BITCOIN_WALLET_WALLET_H