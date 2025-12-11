#include "Base58.h"
#include <algorithm>

// 比特币标准字母表 (去掉了 0, O, I, l)
static const char* pszBase58 = "123456789ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz";

std::string EncodeBase58(const Bytes& input) {
    // 1. 处理前导 0 (Base58 的特性：保留前导 0 为 '1')
    int zeros = 0;
    while (zeros < input.size() && input[zeros] == 0) {
        zeros++;
    }

    // 2. 将字节流转换为大整数 (这里用简单的模拟除法，效率低但逻辑清晰)
    std::vector<unsigned char> b58;
    Bytes data = input; // 拷贝一份

    while (data.size() > zeros) { // 只要还有非0数据
        int carry = 0;
        // 对整个 byte 数组进行 /58 操作，余数就是这一位的 Base58 字符
        for (size_t i = zeros; i < data.size(); i++) {
            int current = (carry << 8) + data[i]; // 上一位余数*256 + 当前位
            data[i] = current / 58;
            carry = current % 58;
        }
        // 去掉除法产生的高位 0
        while (data.size() > zeros && data[zeros] == 0) {
            zeros++;
        }
        b58.push_back(carry);
    }

    // 3. 转换字符
    std::string str;
    // 添加前导 '1'
    for (int i = 0; i < input.size() && input[i] == 0; i++) {
        str += pszBase58[0];
    }
    // 添加计算出的字符 (注意是反向的)
    for (auto it = b58.rbegin(); it != b58.rend(); ++it) {
        str += pszBase58[*it];
    }
    return str;
}

std::string EncodeBase58Check(const Bytes& data) {
    // 1. 计算校验和：Double SHA256 取前 4 字节
    Bytes hash = Hash256(data);
    Bytes checksum(hash.begin(), hash.begin() + 4);

    // 2. 拼接：数据 + 校验和
    Bytes result = data;
    result.insert(result.end(), checksum.begin(), checksum.end());

    // 3. 编码
    return EncodeBase58(result);
}