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