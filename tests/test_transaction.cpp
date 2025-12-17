#include "../src/Core/Transaction.h"
#include "../src/Wallet/Wallet.h"
#include <iostream>
#include <cassert>
#include <cstdio>
#include <openssl/applink.c> 

void TestTransactionSignature() {
    // 清理环境
    remove("alice_tx.dat");

    std::cout << "Creating Wallet for Alice..." << std::endl;
    // [修改] 自动生成
    Wallet aliceWallet("alice_tx.dat");
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
    // Step A: 获取待签名的哈希 (Message Hash)
    // 注意：我们在 Transaction.cpp 里修复了 GetId，它会自动处理签名置空的问题
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

    // 矿工拿到交易
    // [修改] 验证时，我们需要手动计算“未签名状态”的哈希来比对
    // 但由于我们改进了 Transaction::GetId()，它现在永远返回未签名的ID
    // 所以直接调用 GetId() 即可，不需要手动清空签名的繁琐步骤了！
    Bytes checkHash = tx.GetId();

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