#include "../src/Core/Transaction.h"
#include "../src/Wallet/Wallet.h"
#include <iostream>
#include <cassert>

void TestTransactionSignature() {
    std::cout << "Creating Wallet for Alice..." << std::endl;
    Wallet aliceWallet;
    aliceWallet.GenerateNewKey();
    Bytes alicePub = aliceWallet.GetPublicKey();

    // 1. 构造一个模拟交易 (Input)
    // 假设 Alice 之前收到了一笔钱 (TxID: 全0, Index: 0)
    TxIn input;
    input.prevTxId = Bytes(32, 0);
    input.prevIndex = 0;
    input.publicKey = alicePub; // 放入公钥

    // 构造交易体
    Transaction tx;
    tx.inputs.push_back(input);
    tx.outputs.push_back({ 50, "1BobAddress..." }); // 转给 Bob 50 BTC

    // 2. 签名流程
    // 真正比特币的签名极其复杂 (SIGHASH_ALL)，这里演示核心原理：
    // Alice 对“去除签名信息的交易数据”进行哈希，然后签名。

    // Step A: 获取待签名的哈希 (Message Hash)
    // 简易做法：序列化当前交易(此时签名为空) -> Hash256
    Bytes messageHash = tx.GetId();
    std::cout << "Transaction Hash to Sign: " << ToHex(messageHash) << std::endl;

    // Step B: Alice 用私钥签名
    Bytes signature = aliceWallet.Sign(messageHash);
    std::cout << "Signature generated (" << signature.size() << " bytes)" << std::endl;

    // Step C: 将签名填入交易
    tx.inputs[0].signature = signature;

    // --- 模拟网络传输 ---

    // 3. 验证流程 (矿工节点执行)
    std::cout << "Verifying transaction..." << std::endl;
    const TxIn& verifyInput = tx.inputs[0];

    // 矿工拿到交易，提取出数据
    // 注意：验证时，必须重新计算被签名的那个哈希 (即把签名拿掉后的哈希)
    Transaction tempTx = tx;
    tempTx.inputs[0].signature.clear(); // 清空签名以还原原始数据
    Bytes checkHash = tempTx.GetId();

    bool isValid = Wallet::Verify(verifyInput.publicKey, checkHash, verifyInput.signature);

    if (isValid) {
        std::cout << "SUCCESS: Transaction Signature Verified!" << std::endl;
    }
    else {
        std::cerr << "FAILED: Invalid Signature!" << std::endl;
    }
    assert(isValid == true);
}

int main() {
    try {
        TestTransactionSignature();
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}