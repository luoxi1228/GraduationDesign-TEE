#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <cpprest/http_listener.h>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

class Server {
public:
    Server(const std::string& url);
    void start();
    void stop();

private:
    http_listener listener;
    std::atomic<bool> running;
    static std::vector<std::string> buffer;
    static int counter;
    std::mutex buffer_mutex;

    void input_listener();
    void handle_post(http_request request);
};

#endif

