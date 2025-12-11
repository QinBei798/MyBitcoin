#include "Wallet.h"
#include "Base58.h"
#include <iostream>
#include <openssl/ecdsa.h>

Wallet::Wallet() {
    pKey = nullptr;
}

Wallet::~Wallet() {
    if (pKey) {
        EC_KEY_free(pKey);
    }
}

void Wallet::GenerateNewKey() {
    // 1. 创建 secp256k1 曲线
    pKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!pKey) {
        throw std::runtime_error("OpenSSL: Failed to create key structure");
    }

    // 2. 生成密钥对
    if (!EC_KEY_generate_key(pKey)) {
        throw std::runtime_error("OpenSSL: Failed to generate key");
    }

    // 设置为压缩格式 (现代比特币标准，虽然 v0.1 是非压缩的，但我们用现代的更好)
    EC_KEY_set_conv_form(pKey, POINT_CONVERSION_COMPRESSED);
}

Bytes Wallet::GetPublicKey() const {
    if (!pKey) return {};

    // 获取曲线上的点
    const EC_GROUP* group = EC_KEY_get0_group(pKey);
    const EC_POINT* point = EC_KEY_get0_public_key(pKey);

    // 将点转换为字节流
    int length = EC_POINT_point2oct(group, point, POINT_CONVERSION_COMPRESSED, nullptr, 0, nullptr);
    Bytes pubKey(length);
    EC_POINT_point2oct(group, point, POINT_CONVERSION_COMPRESSED, pubKey.data(), length, nullptr);

    return pubKey;
}

std::string Wallet::GetAddress() const {
    // 1. 获取公钥
    Bytes pubKey = GetPublicKey();

    // 2. 计算 Hash160 (SHA256 -> RIPEMD160)
    Bytes pubKeyHash = Hash160(pubKey);

    // 3. 添加版本号 (主网地址以 0x00 开头)
    Bytes versionedPayload;
    versionedPayload.push_back(0x00); // Version byte
    versionedPayload.insert(versionedPayload.end(), pubKeyHash.begin(), pubKeyHash.end());

    // 4. Base58Check 编码 (这一步会自动加校验和)
    return EncodeBase58Check(versionedPayload);
}


Bytes Wallet::Sign(const Bytes& hash) const {
    if (!pKey) throw std::runtime_error("No private key");

    // ECDSA 签名
    ECDSA_SIG* sig = ECDSA_do_sign(hash.data(), hash.size(), pKey);
    if (!sig) return {};

    // 将签名结构体序列化为 DER 字节流 (比特币标准格式)
    int len = i2d_ECDSA_SIG(sig, nullptr);
    Bytes derSig(len);
    unsigned char* p = derSig.data();
    i2d_ECDSA_SIG(sig, &p);

    ECDSA_SIG_free(sig);
    return derSig;
}

bool Wallet::Verify(const Bytes& pubKeyData, const Bytes& hash, const Bytes& signature) {
    // 1. 还原公钥
    const unsigned char* pPub = pubKeyData.data();
    EC_KEY* key = EC_KEY_new_by_curve_name(NID_secp256k1);
    o2i_ECPublicKey(&key, &pPub, pubKeyData.size());

    // 2. 还原签名
    const unsigned char* pSig = signature.data();
    ECDSA_SIG* sig = d2i_ECDSA_SIG(nullptr, &pSig, signature.size());

    // 3. 验证
    int result = ECDSA_do_verify(hash.data(), hash.size(), sig, key);

    ECDSA_SIG_free(sig);
    EC_KEY_free(key);

    return (result == 1);
}