#include "../src/Core/Block.h"
#include <iostream>
#include <cassert>

void TestMining() {
    // 构造一个模拟区块
    Bytes prevHash(32, 0); // 假的前块哈希 (全0)
    Bytes merkleRoot(32, 0); // 假的默克尔根 (全0)

    // 创建区块：版本1，时间戳123456，难度要求 2 个前导零 (对应 0x0000...)
    Block block(1, prevHash, merkleRoot, 123456, 0);

    // 开始挖矿！难度设为 2 (即哈希值必须以 0000 开头)
    // 注意：如果是正式比特币，难度大概是 18-19 个零
    block.Mine(2);

    // 验证结果
    assert(block.CheckPoW(2) == true);
    std::cout << "Mining Test Passed!" << std::endl;
}

int main() {
    TestMining();
    return 0;
}