#include "net/http/websocket/websocket_client.hpp"
#include "net/http/websocket/websocket_pub.hpp"
#include <stdint.h>
#include <string>
#include <iostream>
#include <memory>
#include <unistd.h>
#include <vector>
#include <thread>
#include <queue>
#include <mutex>

using namespace cpp_streamer;
class WsAsyncInfo;

static Logger* s_logger = nullptr;
static std::string s_text;
static std::vector<uint8_t> s_data;
static std::queue<WsAsyncInfo*> s_info_queue;
static std::mutex s_mutex;

void WsAsyncCallback(uv_async_t *handle);

class WsClientWrapper;
class WsAsyncInfo
{
public:
    WsAsyncInfo(WsClientWrapper* ws, int op, const std::string text):ws_client_wrapper_(ws)
                                                                , op_(op)
                                                                , text_(text)
    {
    }
    WsAsyncInfo(WsClientWrapper* ws, int op, const uint8_t* data, size_t len):ws_client_wrapper_(ws)
                                                                , op_(op)
    {
        data_.resize(len);
        memcpy(&data_[0], data, len);
    }
    WsAsyncInfo(WsClientWrapper* ws, int op):ws_client_wrapper_(ws)
                                            , op_(op)
    {
    }
public:
    WsClientWrapper* ws_client_wrapper_;
    int op_ = 0;//0: connect, 1:send text, 2:send bin data
    std::string text_;
    std::vector<uint8_t> data_;
};

class WsClientWrapper : public WebSocketConnectionCallBackI
{
friend void WsAsyncCallback(uv_async_t *handle);

public:
    WsClientWrapper(uv_loop_t* loop, const std::string& hostname, uint16_t port, const std::string& subpath, bool ssl_enable, Logger* logger):logger_(logger)
    {
        uv_async_init(loop, &async_, WsAsyncCallback);
        ws_client_ptr_.reset(new WebSocketClient(loop, hostname, port, subpath, ssl_enable, logger, this));
    }
    ~WsClientWrapper()
    {
    }

public:
    void Connect() {
        std::lock_guard<std::mutex> lock(s_mutex);
        WsAsyncInfo* info = new WsAsyncInfo(this, 0);
        s_info_queue.push(info);
        uv_async_send(&async_);
    }
    void SendText(const std::string& text) {
        std::lock_guard<std::mutex> lock(s_mutex);
        WsAsyncInfo* info = new WsAsyncInfo(this, 1, text);
        s_info_queue.push(info);
        uv_async_send(&async_);
    }
    void SendData(const uint8_t* data, size_t len) {
        std::lock_guard<std::mutex> lock(s_mutex);
        WsAsyncInfo* info = new WsAsyncInfo(this, 2, data, len);
        s_info_queue.push(info);
        uv_async_send(&async_);
    }
    bool IsConnect() {
        return is_connected_;
    }
private:
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
        if (strcmp(text.c_str(), s_text.c_str()) == 0) {
            LogInfof(logger_, "text check ok, count:%d", ++count_);
        } else {
            LogErrorf(logger_, "text check error, text len:%lu, text:%s", text.length(), text.c_str());
        }
    }
    virtual void OnReadData(int code, const uint8_t* data, size_t len) override {
        if (code < 0) {
            LogErrorf(logger_, "websocket read data error:%d", code);
            return;
        }
        if (memcmp((void*)data, (void*)&s_data[0], len) == 0) {
            LogInfof(logger_, "data check ok");
        }  else {
            LogErrorf(logger_, "data check error, data len:%lu", len);
        }
    }
private:
    Logger* logger_ = nullptr;
    uv_async_t async_;
    std::unique_ptr<WebSocketClient> ws_client_ptr_;
    bool is_connected_ = false;
    int count_ = 0;
};

void WsAsyncCallback(uv_async_t *handle) {
    WsAsyncInfo* info = nullptr;
    std::lock_guard<std::mutex> lock(s_mutex);

    while(s_info_queue.size() > 0) {
        info = s_info_queue.front();
        s_info_queue.pop();
        if (info) {
            switch (info->op_)
            {
            case 0:
                info->ws_client_wrapper_->AsyncConnect();
                break;
            case 1:
                info->ws_client_wrapper_->AsyncSendText(info->text_);
                break;
            case 2:
                info->ws_client_wrapper_->AsyncSendData(&info->data_[0], info->data_.size());
                break;
            default:
                break;
            }
            delete info;
        }
    }
}

/*
the demo show how to use the websocket client outside the uv_loop thread.
it use the uv async to ingest data to websocket client which is run in uv_loop thread.
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

        std::thread run_thread([&]{
            client.Connect();

            while(!client.IsConnect()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            s_text = "hello world";
            for (int i = 0; i < 10000; i++) {
                client.SendText(s_text);
                //std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        });
        while (true) {
            uv_run(loop, UV_RUN_DEFAULT);
        }
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}