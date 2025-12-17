#ifndef BITCOIN_WALLET_WALLET_H
#define BITCOIN_WALLET_WALLET_H

#include "../Crypto/Hash.h"
#include "../Core/Transaction.h"
#include <openssl/ec.h>
#include <openssl/obj_mac.h> // NID_secp256k1
#include <string>

// [新增] 前置声明，告诉编译器 Blockchain 是个类 (具体定义在 .cpp 里包含)
class Blockchain;

class Wallet {
private:
    EC_KEY* pKey; // OpenSSL 的 ECC 密钥结构体
    std::string filename; // 钱包文件名

    // [私有化] 真正生成 Key 的逻辑
    void GenerateNewKeyImpl();

public:
    // [修改] 构造函数带默认文件名
    explicit Wallet(const std::string& file = "wallet.dat");
    ~Wallet();

    // [修改] 变为安全版本：如果已有 Key，抛出异常或要求确认
    void GenerateNewKey(bool force = false);

    // [新增] 备份当前钱包到另一个文件
    void Backup(const std::string& backupFilename);

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

    // [新增] 创建一笔转账交易 (自动选币 + 自动找零)
    Transaction CreateTransaction(const std::string& toAddr, int64_t amount, const Blockchain& chain);
};

#endif //BITCOIN_WALLET_WALLET_H