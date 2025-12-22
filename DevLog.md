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

---

## 📅 2025-12-16 | 数据持久化与钱包存储 (Data Persistence)

### 📝 今日进展
- [x] 实现通用二进制序列化模块 `Serialization.h`
- [x] 实现 **区块链持久化**：支持将整条链保存到磁盘 (`blockchain.dat`) 并断点续传
- [x] 优化启动流程：系统启动时自动加载历史区块并重建 UTXO 集合

### 💻 技术细节

**1. 序列化工具 (`src/Utils/Serialization.h`)**
封装了 C++ fstream 的读写操作，支持变长数据存储：
```cpp
// 写入格式: [Size][Data...]
```

**2. 区块链状态恢复 (`src/Core/Blockchain.cpp`)**
加载时不仅读取区块数据，还需重放 (Replay) 整个 UTXO 集合，确保内存状态与磁盘数据一致：
```cpp
void Blockchain::LoadFromDisk(const std::string& filename) {
    // ... 读取区块 ...
    for (const auto& block : chain) {
        if (isGenesis) { /* 初始化 UTXO */ }
        else { ApplyBlockToUTXO(block); } // 重新校验并构建 UTXO
    }
}
```

**3. 钱包密钥存储 (`src/Wallet/Wallet.cpp`)**
利用 OpenSSL 标准 PEM 接口安全存储私钥，兼容性强：
```cpp
PEM_write_ECPrivateKey(fp, pKey, nullptr, ...); // 保存
PEM_read_ECPrivateKey(fp, nullptr, ...);        // 加载
```

**4. 动态难度调整算法 (`src/Core/Blockchain.cpp`)**
实现了每 N 个区块根据出块时间动态调整难度目标 (bits) 的逻辑：
```cpp
if (actualTime < expectedTime / 2) difficulty++; // 太快了，增加难度
else if (actualTime > expectedTime * 2) difficulty--; // 太慢了，降低难度
```

---

## 📅 2025-12-17 | 钱包找零与签名逻辑重构 (Wallet Optimization)

### 📝 今日进展
- [x] 实现 **智能找零机制 (Change Output)**：自动计算输入总额与转账金额的差额，将多余资金返还给自己
- [x] 实现 **动态难度调整**：每 5 个区块根据实际出块时间自动调整挖矿难度

### 💻 技术细节

**1. 解决签名"鸡生蛋"问题 (`src/Core/Transaction.cpp`)**
交易 ID (Hash) 是签名的对象，但签名本身又存放在交易里。
```cpp
// includeSignature=false: 用于计算 TxID (待签名的"原始内容")
// includeSignature=true:  用于网络传输 (包含签名的"完整内容")
Bytes Transaction::Serialize(bool includeSignature) const {
    // ...
    if (includeSignature) {
        PushBytes(data, in.signature);
    } else {
        PushUInt32(data, 0); // 占位或留空
    }
    // ...
}
```

**2. 钱包找零逻辑 (`src/Wallet/Wallet.cpp`)**
UTXO 模型不仅要指定钱给谁，还要指定剩下的钱去哪：
```cpp
// 找零 = 输入总和 - (转账金额 + 手续费)
int64_t change = currentSum - targetTotal;

// 防止粉尘攻击 (Dust): 只有找零大于 546 satoshi 才创建输出
if (change > 546) {
    tx.outputs.push_back({ change, myAddr }); // 返还给自己
}
```

**3. 动态难度调整 (`src/Core/Blockchain.cpp`)**
```cpp
// 规则: 如果太快 (小于期望的一半)，难度加倍
if (actualTimeTaken < expectedTime / 2) {
    return currentDifficulty + 1;
}
// 规则: 如果太慢 (大于期望的 2 倍)，难度减半
else if (actualTimeTaken > expectedTime * 2) {
    return currentDifficulty - 1;
}
```

**4. 架构限制说明**
- **单密钥风险**：当前 Wallet 类与单个 wallet.dat 文件绑定，且只管理一对公私钥
- **地址重用**：找零地址默认使用了发送方地址（虽简单但暴露隐私）。在生产环境中应生成新的找零地址 (HD Wallet)

---

## 📅 2025-12-22 | 系统集成：多线程矿工与 CLI 控制台 (Full System Integration)

### 📝 今日进展
- [x] 实现 **主程序入口 (`main.cpp`)**：整合所有核心模块，提供完整的命令行交互界面。
- [x] 实现 **多线程并发挖矿**：引入后台矿工线程，支持 `start/stop` 异步控制，不影响主线程 CLI 操作。
- [x] 完善 **线程安全机制**：使用全局互斥锁 (`g_mutex`) 保护交易池 (mempool) 和区块链状态，防止数据竞争。
- [x] 实现 **持久化自动加载**：程序启动时自动搜索 `wallet.dat` 和 `blockchain.dat` 并加载历史状态。
- [x] 修复了 CLI 帮助信息中丢失的 `address` 和 `mempool` 命令说明。

### 💻 技术细节

**1. 交互式控制台 (CLI) 功能清单**
系统现在支持以下交互指令：
- `start / stop`: 开启/关闭后台自动挖矿。
- `mine [count]`: 同步手动挖掘指定数量的区块。
- `send <addr> <amount>`: 自动选币、构造、签名交易并加入交易池。
- `balance / address`: 查询余额及本地钱包信息。
- `chain / mempool`: 实时监控区块链增长与待处理交易。

**2. 矿工线程同步策略 (`BackgroundMinerLoop`)**
为了保证算力竞争不破坏内存数据，矿工在每一轮挖矿前后采取了“快照”策略：
- **读取期**：短时间加锁，拷贝当前链顶哈希、难度和交易池到本地变量。
- **挖矿期**：在不持锁的情况下进行 CPU 密集型的 Nonce 碰撞计算。
- **提交期**：找到合法区块后，再次加锁验证链顶是否变动，若无变动则执行上链操作并清空内存交易池。


**3. 并发安全性说明**
- **互斥锁 (`g_mutex`)**：确保 `txPool` 在交易被 `send` 指令添加时，与被矿工线程打包上链时不会发生读写冲突。
- **原子变量 (`g_isMining`)**：主线程修改该标志后，后台线程能立即感知并安全退出循环，避免产生悬挂线程。

### 🚀 项目现状
目前 MyBitcoin 已具备单机版全节点的完整特征：
- [x] 密码学安全（ECDSA + SHA256）
- [x] 动态难度调整（每 5 个块调整一次）
- [x] 完整的 UTXO 状态管理
- [x] 交易与区块的磁盘持久化
- [x] 支持并发操作的控制台
