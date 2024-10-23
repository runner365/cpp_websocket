#include "net/http/websocket/websocket_server.hpp"
#include "net/http/websocket/websocket_session.hpp"
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
#include <map>

using namespace cpp_streamer;

class WebSocketSessionWarpper;

static Logger* s_logger = nullptr;
static std::map<std::string, std::shared_ptr<WebSocketSessionWarpper>> s_sessions_;

void HandleNewWebSocketSession(const std::string& uri, WebSocketSession* session);

int main(int argc, char** argv) {
    char log_file[516];

    int opt = 0;
    bool log_file_ready = false;
    char server_ip[80];
    bool server_ip_ready = false;
    char port_sz[32];
    uint16_t server_port = 0;
    char key_file[256];
    bool key_ready = false;
    char cert_file[256];
    bool cert_ready = false;

    while ((opt = getopt(argc, argv, "s:p:k:c:l:h")) != -1) {
        switch (opt) {
            case 's': strncpy(server_ip, optarg, sizeof(server_ip)); server_ip_ready = true; break;
            case 'p': strncpy(port_sz, optarg, sizeof(port_sz)); server_port = atoi(port_sz); break;
            case 'l': strncpy(log_file, optarg, sizeof(log_file)); log_file_ready = true; break;
            case 'k': strncpy(key_file, optarg, sizeof(key_file)); key_ready = true; break;
            case 'c': strncpy(cert_file, optarg, sizeof(cert_file)); cert_ready = true; break;
            default: 
            {
                printf("Usage: %s [-s websocket host ip]\n\
    [-p websocket host port]\n\
    [-k https key file]\n\
    [-c https cert file]\n\
    [-l log file name]\n",
                    argv[0]); 
                return -1;
            }
        }
    }

    if (!server_ip_ready) {
        sprintf(server_ip, "0.0.0.0");
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
        std::unique_ptr<WebSocketServer> server_ptr;

        if (key_ready && cert_ready) {
            server_ptr.reset(new WebSocketServer(server_port, loop, key_file, cert_file, s_logger));
        } else {
            server_ptr.reset(new WebSocketServer(server_port, loop, s_logger));
        }
        
        server_ptr->AddHandle("/echo", HandleNewWebSocketSession);
        while (true) {
            uv_run(loop, UV_RUN_DEFAULT);
        }
    } catch(const std::exception& e) {
        std::cerr << e.what() << '\n';
    }
    
    return 0;
}

class WebSocketSessionWarpper : public WebSocketSessionCallBackI
{
public:
    WebSocketSessionWarpper(WebSocketSession* session):session_(session)
    {
    }
    virtual ~WebSocketSessionWarpper()
    {
    }

public:
    virtual void OnReadData(int code, const uint8_t* data, size_t len) override {
        if (code < 0) {
            auto iter = s_sessions_.find(session_->GetRemoteAddress());
            if (iter == s_sessions_.end()) {
                LogErrorf(session_->GetLogger(), "fail to get session:%s", session_->GetRemoteAddress().c_str());
                return;
            }
            s_sessions_.erase(session_->GetRemoteAddress());
            return;
        }
        std::string data_str((const char*)data, len);
        LogInfof(s_logger, "websocket receive data:%s", data_str.c_str());
        session_->AsyncWriteData(data, len);
    }
    virtual void OnReadText(int code, const std::string& text) override {
        if (code < 0) {
            auto iter = s_sessions_.find(session_->GetRemoteAddress());
            if (iter == s_sessions_.end()) {
                LogErrorf(session_->GetLogger(), "fail to get session:%s", session_->GetRemoteAddress().c_str());
                return;
            }
            s_sessions_.erase(session_->GetRemoteAddress());
            return;
        }
        LogInfof(s_logger, "websocket receive text:%s", text.c_str());
        session_->AsyncWriteText(text);
    }

private:
    WebSocketSession* session_ = nullptr;
};

void HandleNewWebSocketSession(const std::string& uri, WebSocketSession* session) {
    LogInfof(session->GetLogger(), "new websocket session, uri:%s, remote addr:%s",
        uri.c_str(), session->GetRemoteAddress().c_str());
    
    std::shared_ptr<WebSocketSessionWarpper> session_ptr = std::make_shared<WebSocketSessionWarpper>(session);
    session->SetSessionCallback(session_ptr.get());
    s_sessions_[session->GetRemoteAddress()] = session_ptr;
}