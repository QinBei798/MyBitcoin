#include "Wallet.h"
#include "Base58.h"
#include "../Core/Blockchain.h"
#include <iostream>
#include <fstream> // 用于文件检查
#include <openssl/ecdsa.h>
#include <openssl/pem.h>

Wallet::Wallet(const std::string& file) : filename(file) {
    pKey = nullptr;

    // 1. 尝试打开文件
    FILE* fp = fopen(filename.c_str(), "r");
    if (fp) {
        fclose(fp);
        // 如果文件存在，尝试加载
        if (Load()) {
            return; // 加载成功，直接返回
        }
        else {
            std::cerr << "Warning: Failed to load existing wallet. Creating new one." << std::endl;
        }
    }

    // 2. 如果文件不存在或加载失败，生成新的并保存
    GenerateNewKey();
    Save();
}

Wallet::~Wallet() {
    if (pKey) {
        EC_KEY_free(pKey);
    }
}

// 内部实现，不带检查
void Wallet::GenerateNewKeyImpl() {
    if (pKey) EC_KEY_free(pKey);
    pKey = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!pKey) throw std::runtime_error("OpenSSL error");
    if (!EC_KEY_generate_key(pKey)) throw std::runtime_error("OpenSSL generate error");
    EC_KEY_set_conv_form(pKey, POINT_CONVERSION_COMPRESSED);
}

// [修改] 安全的公开接口
void Wallet::GenerateNewKey(bool force) {
    // 1. 检查内存中是否已有 Key
    if (pKey != nullptr) {
        if (!force) {
            throw std::runtime_error("Safety Error: Wallet already has a key! Use GenerateNewKey(true) to overwrite.");
        }
        // 如果强制覆盖，建议先自动备份 (可选)
        std::cout << "Warning: Overwriting existing key!" << std::endl;
    }

    // 2. 检查硬盘上文件是否存在 (防止误删 wallet.dat)
    std::ifstream f(filename);
    if (f.good() && !force) {
        throw std::runtime_error("Safety Error: Wallet file '" + filename + "' already exists! Aborting.");
    }
    f.close();

    GenerateNewKeyImpl();
    Save(); // 生成后立即保存
}

// [新增] 备份功能
void Wallet::Backup(const std::string& backupFilename) {
    if (!pKey) throw std::runtime_error("No key to backup");

    // 临时修改文件名进行保存
    std::string oldName = filename;
    filename = backupFilename;
    try {
        Save(); // 复用现有的 Save 逻辑
        std::cout << "Wallet backed up to " << backupFilename << std::endl;
    }
    catch (...) {
        filename = oldName; // 恢复文件名
        throw;
    }
    filename = oldName;
}

// 保存私钥到文件 (PEM格式)
void Wallet::Save() {
    if (!pKey) return;

    FILE* fp = fopen(filename.c_str(), "w");
    if (!fp) throw std::runtime_error("Cannot open wallet file for writing");

    // 写入私钥 (参数 nullptr 表示不加密)
    if (!PEM_write_ECPrivateKey(fp, pKey, nullptr, nullptr, 0, nullptr, nullptr)) {
        fclose(fp);
        throw std::runtime_error("Failed to save wallet to file");
    }

    fclose(fp);
    std::cout << "Wallet saved to " << filename << std::endl;
}

// 从文件加载私钥
bool Wallet::Load() {
    FILE* fp = fopen(filename.c_str(), "r");
    if (!fp) return false;

    if (pKey) EC_KEY_free(pKey);

    // 读取私钥
    pKey = PEM_read_ECPrivateKey(fp, nullptr, nullptr, nullptr);
    fclose(fp);

    if (!pKey) return false;

    std::cout << "Wallet loaded from " << filename << std::endl;
    return true;
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

// [新增] 创建交易核心逻辑 (优化版：增加手续费)
Transaction Wallet::CreateTransaction(const std::string& toAddr, int64_t amount, const Blockchain& chain) {
    // 1. 定义手续费 (简化起见固定为 0.00001 BTC)
    // 在真实系统中，这应该根据交易字节大小计算
    const int64_t TX_FEE = 1000;

    // 2. 获取我的所有可用零钱 (UTXO)
    std::string myAddr = GetAddress();
    auto myUTXOs = chain.FindUTXOs(myAddr);

    int64_t currentSum = 0;
    int64_t targetTotal = amount + TX_FEE; // 我们需要凑够：转账金额 + 手续费
    Transaction tx;

    std::cout << "[Wallet] Selecting coins..." << std::endl;

    // 3. 选币算法 (Coin Selection)
    for (const auto& pair : myUTXOs) {
        const std::string& utxoKey = pair.first;
        const TxOut& output = pair.second;

        // 解析 Key: "TxHash_Index"
        size_t splitPos = utxoKey.find('_');
        std::string txIdHex = utxoKey.substr(0, splitPos);
        std::string indexStr = utxoKey.substr(splitPos + 1);

        TxIn input;
        input.prevTxId = FromHex(txIdHex);
        input.prevIndex = std::stoul(indexStr);
        input.publicKey = GetPublicKey();

        tx.inputs.push_back(input);
        currentSum += output.value;

        std::cout << "  - Picked UTXO: " << output.value << " satoshi" << std::endl;

        if (currentSum >= targetTotal) {
            break;
        }
    }

    // 4. 检查余额
    if (currentSum < targetTotal) {
        throw std::runtime_error("Error: Insufficient funds! Have: " + std::to_string(currentSum) +
            " Need: " + std::to_string(targetTotal));
    }

    // 5. 构造 Output 1: 给收款人
    tx.outputs.push_back({ amount, toAddr });

    // 6. 构造 Output 2: 找零
    // 找零 = 当前凑的钱 - (转给别人的 + 手续费)
    // 剩下的那部分 (currentSum - amount - change) 自然就成了 implicit fee
    int64_t change = currentSum - targetTotal;

    // 防止产生“粉尘交易” (Dust): 如果找零太少(比如1 satoshi)，就不找了，全给矿工
    if (change > 546) {
        tx.outputs.push_back({ change, myAddr });
        std::cout << "[Wallet] Change created: " << change << " satoshi return to me." << std::endl;
    }
    else if (change > 0) {
        std::cout << "[Wallet] Change is too small (" << change << "), added to miner fee." << std::endl;
    }

    // 7. 签名
    // 注意：此时 inputs 里的 signature 是空的，GetId() 计算的是未签名的哈希
    Bytes txHash = tx.GetId();
    for (auto& in : tx.inputs) {
        in.signature = Sign(txHash);
    }

    std::cout << "[Wallet] Transaction signed. Fee: " << (currentSum - amount - change) << " satoshi." << std::endl;
    return tx;
}

