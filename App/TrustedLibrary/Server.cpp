#include "Server.h"
#include <fstream>
#include <thread>
#include <chrono>

int Server::counter = 0;
std::vector<std::string> Server::buffer;

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

            std::lock_guard<std::mutex> lock(buffer_mutex);
            buffer.push_back(plaintext);
            counter++;

            if (counter >= 6) {
                std::ofstream file("Transform2_received.txt", std::ios::out);
                if (!file) {
                    request.reply(status_codes::InternalError, "File write error");
                    return;
                }
                for (const auto& line : buffer) {
                    file << line << "\n";
                }
                file.close();
                buffer.clear();
                counter = 0;
            }

            json::value response;
            response["message"] = json::value::string("Data saved successfully");
            request.reply(status_codes::OK, response);
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
