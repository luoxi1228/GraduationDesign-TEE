
#include "Server.h"
#include "FileProcessor.h"
#include "../App.h"
#include "pbc/pbc.h"
#include "Enclave_u.h"  // 包含生成的头文件

#include <cppcodec/base64_rfc4648.hpp> 
#include <iostream>
#include <fstream>
#include <thread>
#include <chrono>


int Server::counter = 0;
std::vector<std::string> Server::buffer;
using namespace cppcodec;
// 定义全局变量
size_t ZR_SIZE = 0;
size_t G1_SIZE = 0;
size_t G2_SIZE = 0;
size_t GT_SIZE = 0;
size_t flag=0;




Server::Server(const std::string& url) : listener(url), running(true) {
    listener.support(methods::POST, std::bind(&Server::handle_post, this, std::placeholders::_1));
}

void Server::start() {
    try {
        listener.open().wait();
        std::cout << "Server is listening on " << listener.uri().to_string() << std::endl;
        std::cout << "Type 'q' to stop the server...\n";

        std::thread input_thread([this] { input_listener(); });

        while (running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        std::cout << "Stopping server..." << std::endl;
        listener.close().wait();
        input_thread.join();
    } catch (const std::exception& e) {
        std::cerr << "Server error: " << e.what() << std::endl;
    }
}

void Server::stop() {
    running = false;
}

void Server::handle_post(http_request request) {
    request.extract_json().then([=](json::value body) {
        try {
            std::string plaintext = body.at("data").as_string();
            if (plaintext.length() >= 1024) {
                request.reply(status_codes::BadRequest, "Data too long");
                return;
            }

            // 线程安全地添加数据到缓存
            {
                std::lock_guard<std::mutex> lock(buffer_mutex);
                buffer.push_back(plaintext);
                counter++;
            }

            // 当累计到 6 行数据时，进行处理
            if (counter == 6) {
                // 将数据写入文件 (一次性写入 6 行)
                std::ofstream file("Transform2_received.txt", std::ios::out | std::ios::trunc);
                if (!file) {
                    request.reply(status_codes::InternalError, "File write error");
                    return;
                }
                {
                    std::lock_guard<std::mutex> lock(buffer_mutex);
                    for (const auto& line : buffer) {
                        file << line << "\n";
                    }
                    buffer.clear();    // 写入后清空缓存
                    counter = 0;       // 重置计数器
                }
                file.close();

                // 调用 get_transform2 并获取字节流
                unsigned char* out = nullptr;
                size_t out_len = 0;
                printf("---------------------------------------------------------\n");
                get_transform2(&out, &out_len);
                std::cout << "TC_len: " << out_len << std::endl;

                if (out && out_len > 0) {
                    // 转换为 Base64
                    std::string base64_str = cppcodec::base64_rfc4648::encode(out, out_len);
                    std::cout << "Base64: " << base64_str << std::endl;

                    // 释放内存
                    free(out);

                    // 发送 Base64 编码后的数据作为 JSON 响应
                    json::value response;
                    response["Message"] = json::value::string(base64_str);
                    request.reply(status_codes::OK, response);
                } else {
                    json::value response;
                    response["Message"] = json::value::string("error");
                    request.reply(status_codes::OK, response);  // Transform2 failed
                }
            } else {
                // 如果未满六行，返回等待响应
                request.reply(status_codes::OK, "Waiting for more data...");
            }
        } catch (const json::json_exception& e) {
            request.reply(status_codes::BadRequest, "Invalid JSON");
        }
    });
}






void Server::input_listener() {
    std::string input;
    while (running) {
        std::cin >> input;
        if (input == "q") {
            running = false;
        }
    }
}


// 把 std::string 转换为 unsigned char*
unsigned char* convertStringToUnsignedChar(std::string& str) {
    size_t len = str.size();
    unsigned char* buffer = (unsigned char*) malloc(len + 1);
    if (!buffer) {
        std::cerr << "Memory allocation failed" << std::endl;
        return nullptr;
    }
    memcpy(buffer, str.c_str(), len);
    buffer[len] = '\0';  // 确保以 NULL 结尾
    return buffer;
}

void get_pairing(pairing_t pairing)
{
    const char *param = "type a   \
    q 40132934874065581357639239301938089130039744463472639389591743372055069245229811691989086088125328594220615378634210894681862132537783020759006156856256486760853214375759294871087842511098137328669742901944483362556170153388301818039153709502326627974456159915072198235053718093631308481607762634120235579251 \
    h 5986502056676971303894401875152023968506744561211054886102595589603460071084910131070137261543726329935522867827513637124526300709663875599084261056444276 \
    r 6703903964971298549787012499102923063739682910296196688861780721860882015036773488400937149083451713845015929093243025426876941560715789883889358865432577 \
    exp2 511  \
    exp1 87   \
    sign1 1  \
    sign0 1";

    // 初始化pbc_param_t
    pbc_param_t par;
    pbc_param_init_set_str(par, param);

    // 初始化pairing_t
    pairing_init_pbc_param(pairing, par);

    //printf("SET_STATIC_SIZE --- \n");
    element_t zr, g1, g2, gt;
    element_init_Zr(zr, pairing);
    element_init_G1(g1, pairing);
    element_init_G2(g2, pairing);
    element_init_GT(gt, pairing);
    element_random(zr);
    element_random(g1);
    element_random(g2);
    element_random(gt);
    ZR_SIZE = element_length_in_bytes(zr);
    G1_SIZE = element_length_in_bytes(g1);
    G2_SIZE = element_length_in_bytes(g2);
    GT_SIZE = element_length_in_bytes(gt);
}


void get_transform2(unsigned char** out, size_t* out_len) {
    std::string PTC_str, HK_str;
    size_t PTC_len, HK_len, TC_len;

    FileProcessor::processFile("Transform2_received.txt", PTC_str, PTC_len, HK_str, HK_len, C1_len);

    pairing_t pairing;
    get_pairing(pairing);

    TC_len = C1_len + GT_SIZE + GT_SIZE;


    unsigned char* PTC_buf = convertStringToUnsignedChar(PTC_str);
    unsigned char* HK_buf = convertStringToUnsignedChar(HK_str);
    unsigned char* TC_buf = (unsigned char*) malloc(TC_len);  // 预分配

    if (!PTC_buf || !HK_buf || !TC_buf) {
        std::cerr << "Memory allocation failed." << std::endl;
        *out = nullptr;
        *out_len = 0;
        if (TC_buf) free(TC_buf);  // 确保释放已分配的内存
        return;
    }
    
    flag=0;
    sgx_status_t ret = transform2(global_eid, &TC_buf, &TC_len, &flag, &HK_buf, &HK_len, &PTC_buf, &PTC_len);
    printf("ret: %d, flag: %zu\n", ret, flag);
    
    if (ret != SGX_SUCCESS || flag != 1) {
        printf("Error: Transform2 failed.\n");
        *out = nullptr;
        *out_len = 0;
        free(TC_buf);  // 防止内存泄漏
    } else {
        *out = TC_buf;
        *out_len = TC_len;
        printf("Transform2 executed successfully.\n");
    }

    free(PTC_buf);
    free(HK_buf);
}
