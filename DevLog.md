# MyBitcoin 开发日志

## 📅 2025-12-10 | 项目初始化

### 📝 今日任务
- [x] 规划项目模块架构
- [x] 配置 Visual Studio 2022 开发环境
- [x] 初始化 Git 仓库并推送到 GitHub

### 🏗️ 项目结构概览
本项目采用模块化设计，核心代码位于 src 目录下，分离了加密、钱包、P2P网络等模块。

```
MyBitcoin/
├── CMakeLists.txt          # 构建配置
├── src/
│   ├── main.cpp            # 程序入口
│   ├── Core/               # 核心逻辑 (区块、交易、链)
│   ├── Crypto/             # 密码学 (Hash, ECC)
│   ├── Wallet/             # 钱包管理
│   ├── Script/             # 脚本引擎
│   ├── P2P/                # 网络通信
│   └── Utils/              # 工具类
└── lib/                    # 第三方库
```
---

## 📅 2025-12-11 | 核心密码学模块 (Crypto) 实现

### 📝 今日进展
- [x] 封装 OpenSSL 的 SHA-256 和 RIPEMD-160 算法
- [x] 实现比特币特有的 **Hash256** (Double SHA-256)，用于工作量证明 (PoW) 和区块哈希计算
- [x] 编写 Utils 辅助工具：实现 Hex 字符串与 Byte 数组的互转

### 💻 技术细节

**1. 基础哈希封装 (`src/Crypto/Hash.h`)**
```cpp
// 双重 SHA-256，比特币中最常用的哈希方式
Bytes Hash256(const Bytes& data) {
    return Sha256(Sha256(data));
}

// 地址生成专用哈希
Bytes Hash160(const Bytes& data) {
    return Ripemd160(Sha256(data));
}
```

**2. 单元测试结果**
针对字符串 "hello" 进行了标准测试向量验证：
- **SHA256**: `2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824`
- **Hash256**: `9595c9df90075148eb06860365df33584b75bff782a510c6cd4883a419833d50`

### ⚠️ 依赖说明
- 需确保开发环境已链接 OpenSSL 静态库或动态库 (libcrypto)
---

## 📅 2025-12-11 | 区块结构与 PoW 挖矿实现

### 📝 今日进展
- [x] 定义比特币 **区块头 (Block Header)** 结构 (共 80 字节)
- [x] 实现 **序列化 (Serialization)** 逻辑，支持 Little-Endian 字节序拼接
    - 计算 Hash256(BlockHeader)
    - 验证哈希值是否满足难度目标 (前导零检查)
- [x] 编写 `test_block.cpp` 验证挖矿流程

### 💻 技术细节

**1. 区块头定义 (`src/Core/Block.h`)**
严格遵循比特币协议，区块头包含 6 个字段：
```cpp
int32_t version;            // 版本号 (4 bytes)
Bytes prevBlockHash;        // 前一区块哈希 (32 bytes)
Bytes merkleRoot;           // Merkle 根 (32 bytes)
uint32_t timestamp;         // 时间戳 (4 bytes)
uint32_t bits;              // 难度目标 (4 bytes)
uint32_t nonce;             // 随机数 (4 bytes)
```

**2. 挖矿核心逻辑 (`src/Core/Block.cpp`)**
```cpp
void Block::Mine(uint32_t difficulty_zeros) {
    nonce = 0;
    while (true) {
        // 1. 检查当前哈希是否满足难度 (前导零个数)
        if (CheckPoW(difficulty_zeros)) {
            std::cout << "Block Mined! Nonce: " << nonce << std::endl;
            break;
        }
        // 2. 尝试下一个 nonce
        nonce++;
        // 3. 处理 nonce 溢出情况 (更新时间戳)
        if (nonce == 0) timestamp++;
    }
}
```

**3. 测试结果**
模拟难度 difficulty=2 (哈希前两位为 0)，成功挖出区块：
- **Target**: 2 leading zeros
- **Result**: Mining Test Passed!
---

## 📅 2025-12-11 | 交易结构与数字签名 (Transaction & ECDSA)

### 📝 今日进展
- [x] 定义 **交易 (Transaction)** 核心结构：`TxIn` (输入) 与 `TxOut` (输出)
- [x] 完善 **钱包 (Wallet)** 功能：集成 OpenSSL 实现 **ECDSA 签名**与**验签**
- [x] 实现交易序列化 (`Serialize`) 与 交易ID计算 (`GetId`)
- [x] 编写 `test_transaction.cpp` 模拟 "Alice 签名 -^> 矿工验证" 的完整流程

### 💻 技术细节

**1. 交易模型 (`src/Core/Transaction.h`)**
采用了经典的 UTXO 模型结构：
```cpp
struct TxIn {
    Bytes prevTxId;       // 引用上一笔交易 Hash
    uint32_t prevIndex;   // 引用上一笔交易的 Output 索引
    Bytes signature;      // 解锁脚本 (ScriptSig)
    Bytes publicKey;      // 公钥
};

struct TxOut {
    int64_t value;        // 转账金额 (Satoshi)
    std::string address;  // 锁定脚本 (ScriptPubKey)
};
```
---

## 📅 2025-12-13 | 区块链管理与默克尔树 (Blockchain & Merkle Tree)

### 📝 今日进展
- [x] 实现 **Merkle Tree** 根哈希计算逻辑，确保区块内交易数据的完整性
- [x] 实现 **Blockchain** 类：
    - 创世区块 (Genesis Block) 的自动生成

### 💻 技术细节

**1. 默克尔树计算 (`src/Core/Merkle.cpp`)**
实现了标准的比特币 Merkle Tree 算法：自底向上，两两配对哈希
```cpp
// 核心逻辑：层层向上计算 Hash256(Left + Right)
while (hashes.size() > 1) {
    if (hashes.size() % 2 != 0) hashes.push_back(hashes.back()); // 奇数补齐

    for (size_t i = 0; i < hashes.size(); i += 2) {
        Bytes concat = hashes[i]; 
        concat.insert(concat.end(), hashes[i+1].begin(), hashes[i+1].end());
        newLevel.push_back(Hash256(concat)); // 双重哈希
    }
    hashes = newLevel;
}
```
---

## 📅 2025-12-15 | UTXO 模型与防双花机制 (UTXO & Double Spend)

### 📝 今日进展
- [x] 重构 `Blockchain` 类，引入 **UTXO Set** (未花费交易输出集合) 来追踪全网资金状态

### 💻 技术细节

**1. UTXO 核心设计 (`src/Core/Blockchain.h`)**
使用 `std::map` 维护全网"活着的"资金：
```cpp
// Key: "TxID_OutputIndex", Value: TxOut (金额+地址)
```

---

**更新日期：2025-12-15**  
**项目状态：开发中**
