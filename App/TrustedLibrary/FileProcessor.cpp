#include "FileProcessor.h"
#include <bitset>
#include <algorithm>
#include <cstring>
#include <iostream>


// Base64 解码表
static const string base64_chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

bool is_base64(unsigned char c) {
    return (isalnum(c) || (c == '+') || (c == '/'));
}

// Base64 解码函数
vector<unsigned char> base64_decode(const string& encoded_string) {
    int in_len = encoded_string.size();
    int i = 0, j = 0, in_ = 0;
    unsigned char char_array_4[4], char_array_3[3];
    vector<unsigned char> ret;

    while (in_len-- && (encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
        char_array_4[i++] = encoded_string[in_]; in_++;
        if (i == 4) {
            for (i = 0; i < 4; i++)
                char_array_4[i] = base64_chars.find(char_array_4[i]);

            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

            for (i = 0; i < 3; i++)
                ret.push_back(char_array_3[i]);
            i = 0;
        }
    }

    if (i) {
        for (j = i; j < 4; j++)
            char_array_4[j] = 0;

        for (j = 0; j < 4; j++)
            char_array_4[j] = base64_chars.find(char_array_4[j]);

        char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
        char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
        char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

        for (j = 0; j < (i - 1); j++) ret.push_back(char_array_3[j]);
    }

    return ret;
}

// 将字节数组转换为二进制字符串
string bytesToBinaryString(const vector<unsigned char>& bytes) {
    string binaryStr;
    for (unsigned char byte : bytes) {
        binaryStr += bitset<8>(byte).to_string(); // 每个字节转换为 8 位二进制
    }
    return binaryStr;
}

// 判断是否是二进制字符串（仅由 '0' 和 '1' 组成，且长度是 8 的倍数）
bool isBinaryString(const string& str) {
    return !str.empty() && all_of(str.begin(), str.end(), [](char c) { return c == '0' || c == '1'; }) && (str.size() % 8 == 0);
}

// 读取文件并转换每一行数据并拼接到一起
void FileProcessor::processFile(const string& filePath, string& PTC_str, size_t& PTC_len, string& HK_str, size_t& HK_len, size_t& C1_len) {
    ifstream file(filePath);
    if (!file) {
        cerr << "Error opening file: " << filePath << endl;
        return;
    }

    string line;
    PTC_str.clear();
    HK_str.clear();
    
    for (int i = 0; i < 6; ++i) {  // 已知文件有 6 行
        if (!getline(file, line)) {
            cerr << "Error: Unexpected end of file." << endl;
            return;
        }

        vector<unsigned char> decodedBytes;
        decodedBytes = base64_decode(line);  // Base64 解码得到字节流   
        if (i==1) {
            //decodedBytes.assign(line.begin(), line.end());  // 直接转换成字节流
            C1_len = decodedBytes.size();  // 计算 C1 的字节数
        } 

        // 前 4 行拼接到 PTC_str
        if (i < 4) {
            PTC_str.append(decodedBytes.begin(), decodedBytes.end());
        } else {  // 后 2 行拼接到 HK_str
            HK_str.append(decodedBytes.begin(), decodedBytes.end());
        }
    }

    file.close();

    // 计算 **字节数**
    PTC_len = PTC_str.size();  // PTC 实际的字节大小
    HK_len = HK_str.size();  // HK 实际的字节大小
}


