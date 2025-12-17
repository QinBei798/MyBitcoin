#ifndef BITCOIN_UTILS_SERIALIZATION_H
#define BITCOIN_UTILS_SERIALIZATION_H

#include <iostream>
#include <vector>
#include <string>
#include <cstdint>

// 写入整数 (小端序)
template<typename T>
void WriteInt(std::ostream& os, T value) {
    os.write(reinterpret_cast<const char*>(&value), sizeof(T));
}

// 读取整数
template<typename T>
void ReadInt(std::istream& is, T& value) {
    is.read(reinterpret_cast<char*>(&value), sizeof(T));
}

// 写入字节数组: [Size][Data...]
inline void WriteBytes(std::ostream& os, const std::vector<uint8_t>& data) {
    uint32_t size = (uint32_t)data.size();
    WriteInt(os, size);
    os.write(reinterpret_cast<const char*>(data.data()), size);
}

// 读取字节数组
inline void ReadBytes(std::istream& is, std::vector<uint8_t>& data) {
    uint32_t size;
    ReadInt(is, size);
    data.resize(size);
    is.read(reinterpret_cast<char*>(data.data()), size);
}

// 写入字符串
inline void WriteString(std::ostream& os, const std::string& str) {
    uint32_t size = (uint32_t)str.size();
    WriteInt(os, size);
    os.write(str.data(), size);
}

// 读取字符串
inline void ReadString(std::istream& is, std::string& str) {
    uint32_t size;
    ReadInt(is, size);
    str.resize(size);
    is.read(&str[0], size);
}

#endif //BITCOIN_UTILS_SERIALIZATION_H