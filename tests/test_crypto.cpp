#include <iostream>
#include <cassert>
#include "Crypto/Hash.h"

void TestSha256() {
    // 测试用例 1: "hello"
    // 预期结果: 2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824
    std::string input = "hello";
    Bytes data = ToBytes(input);
    Bytes hash = Sha256(data);
    std::string hex = ToHex(hash);

    std::cout << "SHA256('hello'): " << hex << std::endl;
    assert(hex == "2cf24dba5fb0a30e26e83b2ac5b9e29e1b161e5c1fa7425e73043362938b9824");
}

void TestHash256() {
    // 测试用例 2: "hello" 的双重哈希
    // SHA256("hello") = 2cf24...
    // SHA256(2cf24...) = 9595c9df90075148eb06860365df33584b75bff782a510c6cd4883a419833d50
    // 注意：这里的输入是字符串"hello"，不是第一次哈希后的字节
    std::string input = "hello";
    Bytes data = ToBytes(input);
    Bytes hash = Hash256(data);
    std::string hex = ToHex(hash);

    std::cout << "Hash256('hello'): " << hex << std::endl;
    assert(hex == "9595c9df90075148eb06860365df33584b75bff782a510c6cd4883a419833d50");
}

int main() {
    try {
        TestSha256();
        TestHash256();
        std::cout << "All Crypto Tests Passed!" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Test Failed: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}