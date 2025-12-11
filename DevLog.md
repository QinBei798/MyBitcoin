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
├── include/                # 公共头文件
├── tests/                  # 单元测试
└── lib/                    # 第三方库

### 🚀 下一步计划
1. 定义 Block 类头文件 (`src/Core/Block.h`)
2. 实现 SHA-256 哈希函数的封装
"@

# MyBitcoin 开发日志

## 📅 2025-12-10 | 项目初始化

### 📝 今日任务
- [x] 规划项目模块架构
- [x] 配置 Visual Studio 2022 开发环境
- [x] 使用脚本自动生成项目目录和基础文件
- [x] 初始化 Git 仓库并推送到 GitHub

### 🏗️ 项目结构概览
本项目采用模块化设计，核心代码位于 `src` 目录下，分离了加密、钱包、P2P网络等模块。
当前版本 (`v0.1.0`) 目录结构如下：
```text
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
├── include/                # 公共头文件
├── tests/                  # 单元测试
└── lib/                    # 第三方库

### 🚀 下一步计划
1. 定义 Block 类头文件 (`src/Core/Block.h`)
2. 实现 SHA-256 哈希函数的封装
"@

---

## 📅 2025-12-11 | 区块结构与 PoW 挖矿实现

### 📝 今日进展
- [x] 定义比特币 **区块头 (Block Header)** 结构 (共 80 字节)。
- [x] 实现 **序列化 (Serialization)** 逻辑，支持 Little-Endian 字节序拼接。
- [x] 实现 **工作量证明 (PoW)** 挖矿算法：
    - 不断递增 `nonce`。
    - 计算 `Hash256(BlockHeader)`。
    - 验证哈希值是否满足难度目标 (前导零检查)。
- [x] 编写 `test_block.cpp` 验证挖矿流程。

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