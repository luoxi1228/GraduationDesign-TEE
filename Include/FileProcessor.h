#ifndef FILE_PROCESSOR_H
#define FILE_PROCESSOR_H

#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

class FileProcessor {
public:
    static void processFile(const std::string& filePath, std::string& PTC_str, size_t& PTC_len, 
                            std::string& HK_str, size_t& HK_len, size_t& C1_len);
};

#endif // FILE_PROCESSOR_H
