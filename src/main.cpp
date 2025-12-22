// src/main.cpp
// MyBitcoin — 程序入口与命令行交互 (带多线程挖矿示例)
//
// 说明：本文件负责启动钱包与区块链，提供一个简单的 CLI 用于控制
//       后台自动挖矿、手动挖矿、发送交易、查询余额/链状态等。
//       特别注意并发与互斥：全局交易池 `txPool` 在主线程与后台矿工线程之间共享，
//       必须通过 `g_mutex` 保护访问；使用 `g_isMining` 原子标志控制矿工生命周期。
//       保留了 Windows 与 OpenSSL 的兼容处理（applink）。
//
// 编译要求：C++14/17，OpenSSL（包含 applink），使用 CMake/Ninja 构建。

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <iomanip>
#include <ctime>
#include <thread>
#include <atomic>
#include <mutex>

// [关键] 解决 Windows OpenSSL 兼容性
// - 在 Windows 平台使用 OpenSSL 时，需要链接 `openssl/applink.c` 来解决 CRT I/O 回调的差异。
// - 该文件必须在包含 OpenSSL 头文件和使用 OpenSSL API 的文件中链接（通常只需在程序入口处包含一次）。
#include <openssl/applink.c>

#include "Core/Blockchain.h"
#include "Wallet/Wallet.h"

// --- 全局共享资源 ---
// 这些对象在主线程与后台矿工线程之间共享，必须小心并发访问。
std::vector<Transaction> txPool;         // 交易池（mempool），保存尚未上链的交易
std::mutex g_mutex;                      // 全局互斥锁：保护 txPool、区块链状态读取/写入等共享资源
std::atomic<bool> g_isMining{ false };   // 后台挖矿开关（原子变量，避免数据竞争）
std::thread g_minerThread;               // 后台挖矿线程对象（用于 join/管理线程生命周期）

// 辅助：打印帮助信息（CLI 命令说明）
void PrintHelp() {
    std::cout << "\n=== MyBitcoin CLI v2.1 (Fixed) ===" << std::endl;
    std::cout << "  start                  Start background auto-mining" << std::endl;
    std::cout << "  stop                   Stop background auto-mining" << std::endl;
    std::cout << "  mine [count]           Manually mine N blocks (Main thread)" << std::endl;
    std::cout << "  send <addr> <amount>   Send coins (adds tx to mempool)" << std::endl;
    std::cout << "  balance [addr]         Check balance" << std::endl;
    std::cout << "  address                Show my wallet address" << std::endl; // [修复] 显示帮助
    std::cout << "  chain                  Print blockchain status" << std::endl;
    std::cout << "  mempool                Show pending transactions" << std::endl;
    std::cout << "  help                   Show this help" << std::endl;
    std::cout << "  exit                   Save and exit" << std::endl;
    std::cout << "=======================================\n" << std::endl;
}

// --- 后台挖矿线程函数 ---
// 参数说明：
//   pWallet - 指向主程序中 Wallet 的指针（用于获取矿工地址、签名交易等）
//   pChain  - 指向 Blockchain 的指针（用于读取链状态并在成功挖到区块后上链）
//
// 线程行为：
//   1. 循环检查 g_isMining（由主线程控制）
//   2. 在加锁保护下拷贝当前需要的数据（最新区块、难度、当前 mempool、矿工地址）到本地变量
//   3. 构造 coinbase 交易并把 mempool 中的交易收集到新区块中
//   4. 调用 FinalizeAndMine 执行计算 merkle root 并开始挖矿（阻塞直到找到合法 nonce）
//   5. 再次加锁判断前置块是否未被其他线程替换，若未替换则上链并在需要时清空 txPool
//   6. 轻度休眠后继续下一轮
void BackgroundMinerLoop(Wallet* pWallet, Blockchain* pChain) {
    std::cout << "[System] Background miner started." << std::endl;

    while (g_isMining) {
        // 1. 准备数据 (短时间加锁读取当前状态并拷贝到本地变量)
        //    - 使用局部拷贝避免长期持有锁（挖矿过程可能耗时很长）
        Block prevBlock(0, {}, {}, 0, 0);
        uint32_t difficulty = 0;
        std::vector<Transaction> currentTxs;
        std::string minerAddr;

        {
            // 只在此作用域内持有锁以安全读取共享状态
            std::lock_guard<std::mutex> lock(g_mutex);
            prevBlock = pChain->GetLatestBlock();   // 获取当前链顶
            difficulty = pChain->GetDifficulty();   // 获取当前难度（本实现用简单的 leading-zero 检查）
            currentTxs = txPool;                    // 拷贝当前 mempool
            minerAddr = pWallet->GetAddress();      // 获取矿工地址（coinbase 收益地址）
        } // 解锁 - 释放互斥锁，接下来可以不持锁地挖矿

        // 2. 构造新区块（填入 header 的固定字段，merkle root 会在 FinalizeAndMine 中计算）
        Block newBlock(1, prevBlock.GetHash(), Bytes(32, 0), (uint32_t)time(nullptr), difficulty);

        // 构造 coinbase（创币）交易：
        // - coinbase Tx 的输入通常设置 prevIndex = 0xFFFFFFFF 来表示特殊输入（无前序交易）
        // - 输出设置为给矿工地址一定数量的 satoshi（此处示例为 50 BTC = 5,000,000,000 satoshis）
        Transaction coinbase;
        TxIn coinIn; coinIn.prevIndex = 0xFFFFFFFF; // 特殊标记：coinbase 输入
        coinbase.inputs.push_back(coinIn);
        coinbase.outputs.push_back({ 5000000000, minerAddr }); // 奖励
        newBlock.AddTransaction(coinbase);

        // 将 mempool 中的交易添加到区块（顺序按 currentTxs 的拷贝）
        for (const auto& tx : currentTxs) {
            newBlock.AddTransaction(tx);
        }

        // 3. 挖矿（FinalizeAndMine 会先计算 Merkle Root，然后执行 Mine）
        //    - 挖矿是 CPU 密集型且阻塞的操作，期间不应持有全局锁
        newBlock.FinalizeAndMine(difficulty);

        // 4. 尝试上链（必须再次短暂持锁，确保链在这段时间内没有变化）
        if (g_isMining) { // 再次检查，防止在挖矿期间外部请求停止矿工
            std::lock_guard<std::mutex> lock(g_mutex);
            // 检查父块哈希是否仍与链顶一致，避免因重组/其他线程已上链导致冲突
            if (newBlock.prevBlockHash == pChain->GetLatestBlock().GetHash()) {
                pChain->AddBlock(newBlock); // 将新区块添加到链上（并更新 UTXO 等）
                // 如果之前有 mempool 中的交易已经包含在区块内，则清空共享 txPool（本示例简单处理）
                if (!currentTxs.empty()) {
                    txPool.clear();
                }
                // 注意：此处暂用 difficulty 代替高度显示（实际应使用链高度或 block.GetHeight()）
                std::cout << "[AutoMiner] Block added! Height: " << pChain->GetDifficulty() << std::endl;
            }
            else {
                // 父块不匹配，说明链顶已被别人更新（产生冲突），该区块弃用
                std::cout << "[AutoMiner] Stale block dropped." << std::endl;
            }
        }

        // 轻度休眠，避免热循环占用 100% CPU（也给其他线程调度机会）
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "[System] Background miner stopped." << std::endl;
}

int main() {
    std::cout << "Initializing MyBitcoin Core (Multi-threaded)..." << std::endl;

    // Wallet 初始化：
    // - Wallet 构造函数可能会尝试从磁盘加载 `wallet.dat`，或在不存在时创建新密钥对
    Wallet myWallet("wallet.dat");
    std::cout << "Wallet loaded. Address: " << myWallet.GetAddress() << std::endl;

    // Blockchain 初始化并从磁盘加载历史数据（如果存在）
    Blockchain chain(myWallet.GetAddress());
    chain.LoadFromDisk("blockchain.dat");

    std::cout << "\nWelcome! Type 'start' to enable auto-mining." << std::endl;

    // 简单的交互式命令循环（CLI）
    std::string line;
    while (true) {
        if (!std::getline(std::cin, line)) break; // 处理 EOF 或输入流关闭
        if (line.empty()) continue;

        std::stringstream ss(line);
        std::string cmd;
        ss >> cmd;

        try {
            if (cmd == "exit") {
                // 退出流程：先停止后台矿工（若正在运行），等待线程结束，然后保存链数据并退出
                if (g_isMining) {
                    std::cout << "Stopping miner..." << std::endl;
                    g_isMining = false; // 通知后台线程停止
                    if (g_minerThread.joinable()) g_minerThread.join(); // 等待线程安全退出
                }
                // 保存链状态到磁盘（加锁保证一致性）
                std::lock_guard<std::mutex> lock(g_mutex);
                chain.SaveToDisk("blockchain.dat");
                std::cout << "Bye!" << std::endl;
                break;
            }
            else if (cmd == "help") {
                PrintHelp();
            }
            else if (cmd == "start") {
                // 启动后台挖矿线程（如果尚未运行）
                if (g_isMining) {
                    std::cout << "Miner is already running!" << std::endl;
                }
                else {
                    g_isMining = true;
                    // 使用 std::thread 启动后台函数，传递 Wallet 和 Blockchain 的指针
                    g_minerThread = std::thread(BackgroundMinerLoop, &myWallet, &chain);
                }
            }
            else if (cmd == "stop") {
                // 停止并等待后台矿工线程结束
                if (!g_isMining) {
                    std::cout << "Miner is not running." << std::endl;
                }
                else {
                    g_isMining = false; // 通知后台线程退出循环
                    std::cout << "Stopping miner (waiting for current block)..." << std::endl;
                    if (g_minerThread.joinable()) g_minerThread.join();
                }
            }
            else if (cmd == "mine") {
                // 主线程手动挖矿：在主线程中同步挖矿指定数量的区块
                int count = 1;
                if (ss >> count) if (count < 1) count = 1;

                // 在进行手动挖矿时持锁，防止后台矿工同时修改链或 txPool。
                // 注意：这会阻塞后台矿工获取锁，但我们在此示例中选择简化处理。
                std::lock_guard<std::mutex> lock(g_mutex);
                std::cout << "[Manual] Mining " << count << " blocks..." << std::endl;

                for (int i = 0; i < count; ++i) {
                    uint32_t diff = chain.GetDifficulty();
                    Block newBlock(1, chain.GetLatestBlock().GetHash(), Bytes(32, 0), (uint32_t)time(nullptr), diff);

                    // 构造 coinbase 并加入新区块
                    Transaction coinbase;
                    coinbase.inputs.push_back({ Bytes(32,0), 0xFFFFFFFF });
                    coinbase.outputs.push_back({ 5000000000, myWallet.GetAddress() });
                    newBlock.AddTransaction(coinbase);

                    // 将当前 mempool 的交易打包进新区块
                    for (const auto& tx : txPool) newBlock.AddTransaction(tx);

                    // 如果这是第一次打包，则清空共享 txPool（此处逻辑与后台矿工不同）
                    if (i == 0) txPool.clear();

                    // 计算 merkle root 并挖矿（阻塞）
                    newBlock.FinalizeAndMine(diff);

                    // 将区块添加到链上（此处已持锁）
                    chain.AddBlock(newBlock);
                }
                std::cout << "[Manual] Done." << std::endl;
            }
            else if (cmd == "balance") {
                // 查询余额：如果提供地址则查询该地址，否则查询本地钱包地址
                std::string addr;
                std::lock_guard<std::mutex> lock(g_mutex);
                if (ss >> addr) {
                    std::cout << "Balance: " << chain.GetBalance(addr) << std::endl;
                }
                else {
                    std::cout << "My Balance: " << chain.GetBalance(myWallet.GetAddress()) << std::endl;
                }
            }
            // [修复] 找回丢失的 address 命令
            else if (cmd == "address") {
                // 显示钱包地址与公钥（ToHex 用于将字节序列转为可读十六进制）
                std::lock_guard<std::mutex> lock(g_mutex);
                std::cout << "Address:    " << myWallet.GetAddress() << std::endl;
                std::cout << "Public Key: " << ToHex(myWallet.GetPublicKey()) << std::endl;
            }
            else if (cmd == "chain") {
                // 打印链的详细信息与当前难度
                std::lock_guard<std::mutex> lock(g_mutex);
                chain.PrintChain();
                std::cout << "Difficulty: " << chain.GetDifficulty() << std::endl;
            }
            // [修复] 找回丢失的 mempool 命令
            else if (cmd == "mempool") {
                // 列出 mempool 中的交易摘要（TxID、输入输出数量）
                std::lock_guard<std::mutex> lock(g_mutex);
                std::cout << "Pending Transactions: " << txPool.size() << std::endl;
                for (const auto& tx : txPool) {
                    std::cout << " - TxID: " << ToHex(tx.GetId())
                        << " | In: " << tx.inputs.size()
                        << " | Out: " << tx.outputs.size() << std::endl;
                }
            }
            else if (cmd == "send") {
                // 构造并广播交易（在本地 mempool 中添加）
                std::string target;
                int64_t amount;
                if (ss >> target >> amount) {
                    std::lock_guard<std::mutex> lock(g_mutex);
                    // Wallet::CreateTransaction：会根据 UTXO 选择输入并对交易签名
                    Transaction tx = myWallet.CreateTransaction(target, amount, chain);
                    txPool.push_back(tx); // 将交易放入全局 mempool（受锁保护）
                    std::cout << "Transaction added to pool. Wait for miner." << std::endl;
                }
                else {
                    std::cout << "Usage: send <addr> <amount>" << std::endl;
                }
            }
            else {
                std::cout << "Unknown command." << std::endl;
            }
        }
        catch (const std::exception& e) {
            // 捕获并打印运行时错误（例如交易构造失败、磁盘 IO 异常等）
            std::cout << "Error: " << e.what() << std::endl;
        }
    }

    return 0;
}