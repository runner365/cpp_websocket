#include "websocket_server.hpp"
#include "websocket_session.hpp"

namespace cpp_streamer
{
WebSocketServer::WebSocketServer(uint16_t port, uv_loop_t* loop, Logger* logger):TimerInterface(loop, 5*1000)
                                                                            , port_(port)
                                                                            , loop_(loop)
                                                                            , logger_(logger)
{
    server_ptr_.reset(new TcpServer(loop_, port_, this));
    StartTimer();
    LogInfof(logger_, "WebSocketServer construct, port:%d", port);
}

WebSocketServer::WebSocketServer(uint16_t port,
                            uv_loop_t* loop, 
                            const std::string& key_file, 
                            const std::string& cert_file, 
                            Logger* logger):TimerInterface(loop, 5*1000)
                                        , port_(port)
                                        , loop_(loop)
                                        , logger_(logger)
                                        , key_file_(key_file)
                                        , cert_file_(cert_file)
{
    server_ptr_.reset(new TcpServer(loop_, port_, this));
    StartTimer();
    LogInfof(logger_, "WebSocketServer construct, port:%d, key file:%s, cert file:%s", port, key_file_.c_str(), cert_file_.c_str());
}

WebSocketServer::~WebSocketServer()
{
}

void WebSocketServer::OnAccept(int ret_code, uv_loop_t* loop, uv_stream_t* handle) {
    std::shared_ptr<WebSocketSession> session_ptr;

    if (ret_code < 0) {
        return;
    }
    if (key_file_.empty() || cert_file_.empty()) {
        session_ptr.reset(new WebSocketSession(loop, handle, this, logger_));
    } else {
        session_ptr.reset(new WebSocketSession(loop, handle, this, key_file_, cert_file_, logger_));
    }
    sessions_[session_ptr->GetRemoteAddress()] = session_ptr;
}


void WebSocketServer::OnTimer() {
    int64_t now_ms = now_millisec();
    auto iter = sessions_.begin();

    while (iter != sessions_.end()) {
        if (now_ms - iter->second->GetLastPongMs() > 15 * 1000) {
            LogInfof(logger_, "ping/pong is timeout, remove ws session:%s", iter->second->GetRemoteAddress().c_str());
            iter = sessions_.erase(iter);
        } else {
            iter++;
        }
    }
}

void WebSocketServer::AddHandle(const std::string& uri, HandleWebSocketPtr handle_ptr) {
    uri_handles_[uri] = handle_ptr;
}

HandleWebSocketPtr WebSocketServer::GetHandle(const std::string& uri) {
    HandleWebSocketPtr func_ptr = nullptr;

    auto iter = uri_handles_.find(uri);
    if (iter == uri_handles_.end()) {
        return func_ptr;
    }
    func_ptr = iter->second;
    return func_ptr;
}

}