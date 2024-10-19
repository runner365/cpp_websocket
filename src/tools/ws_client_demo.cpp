#include "net/http/websocket/websocket_client.hpp"
#include "net/http/websocket/websocket_pub.hpp"
#include <stdint.h>
#include <string>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <vector>

using namespace cpp_streamer;

static Logger* s_logger = nullptr;

class WsClientWrapper : public WebSocketConnectionCallBackI
{
friend void WsAsyncCallback(uv_async_t *handle);

public:
    WsClientWrapper(uv_loop_t* loop, const std::string& hostname, uint16_t port, const std::string& subpath, bool ssl_enable, Logger* logger):logger_(logger)
    {
        ws_client_ptr_.reset(new WebSocketClient(loop, hostname, port, subpath, ssl_enable, logger, this));
        text_ = "0123456789";
        data_.resize(text_.length());
        memcpy(&data_[0], text_.c_str(), text_.length());
    }
    ~WsClientWrapper()
    {
    }

public:
    void AsyncConnect() {
        if (is_connected_) {
            LogErrorf(logger_, "websocket is connected, you needn't connect again.");
            return;
        }
        LogInfof(logger_, "websocket start connecting...");
        ws_client_ptr_->AsyncConnect();
    }
    void AsyncSendText(const std::string& text) {
        ws_client_ptr_->AsyncWriteText(text);
    }
    void AsyncSendData(const uint8_t* data, size_t len) {
        ws_client_ptr_->AsyncWriteData(data, len);
    }
protected:
    virtual void OnConnection() override {
        is_connected_ = true;
        LogInfof(logger_, "websocket is connected...");
        AsyncSendText(text_);
    }
    virtual void OnClose(int code, const std::string& desc) override {
        is_connected_ = false;
        LogInfof(logger_, "websocket is closed, code:%d, desc:%s", code, desc.c_str());
    }
    virtual void OnReadText(int code, const std::string& text) override {
        if (code < 0) {
            LogErrorf(logger_, "websocket read text error:%d", code);
            return;
        }
        if (strcmp(text.c_str(), text_.c_str()) == 0) {
            LogInfof(logger_, "text check ok, count:%d", ++count_);
            if (count_ < 1000) {
                AsyncSendData(&data_[0], data_.size());
            }
        } else {
            LogErrorf(logger_, "text check error, text len:%lu, text:%s", text.length(), text.c_str());
        }
    }
    virtual void OnReadData(int code, const uint8_t* data, size_t len) override {
        if (code < 0) {
            LogErrorf(logger_, "websocket read data error:%d", code);
            return;
        }
        if (memcmp((void*)data, (void*)&data_[0], len) == 0) {
            LogInfof(logger_, "data check ok");
            AsyncSendText(text_);
        }  else {
            LogErrorf(logger_, "data check error, data len:%lu", len);
        }
    }
private:
    Logger* logger_ = nullptr;
    std::unique_ptr<WebSocketClient> ws_client_ptr_;
    bool is_connected_ = false;

private:
    std::string text_;
    std::vector<uint8_t> data_;
    int count_ = 0;
};

/*
the demo show how to use the websocket client int the uv loop thread.
*/
int main(int argc, char** argv) {
    char log_file[516];

    int opt = 0;
    bool log_file_ready = false;
    char server_ip[80];
    bool server_ip_ready = false;
    char port_sz[32];
    uint16_t server_port = 0;

    while ((opt = getopt(argc, argv, "s:p:l:h")) != -1) {
        switch (opt) {
            case 's': strncpy(server_ip, optarg, sizeof(server_ip)); server_ip_ready = true; break;
            case 'p': strncpy(port_sz, optarg, sizeof(port_sz)); server_port = atoi(port_sz); break;
            case 'l': strncpy(log_file, optarg, sizeof(log_file)); log_file_ready = true; break;
            case 'h':
            default: 
            {
                printf("Usage: %s [-s websocket host ip]\n\
    [-p websocket host port]\n\
    [-l log file name]\n",
                    argv[0]); 
                return -1;
            }
        }
    }

    if (!server_ip_ready) {
        std::cout << "please input server ip.\r\n";
        return -1;
    }
    if (server_port == 0) {
        std::cout << "please input server port.\r\n";
        return -1;
    }
    s_logger = new Logger();
    if (log_file_ready) {
        s_logger->SetFilename(std::string(log_file));
    }
    uv_loop_t* loop = uv_default_loop();
    try {
        WsClientWrapper client(loop, server_ip, server_port, "/echo", false, s_logger);

        client.AsyncConnect();

        while (true) {
            uv_run(loop, UV_RUN_DEFAULT);
        }
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}