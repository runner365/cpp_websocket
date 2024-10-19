#ifndef WEBSOCKET_SESSION_HPP
#define WEBSOCKET_SESSION_HPP

#include "tcp/tcp_session.hpp"
#include "utils/data_buffer.hpp"
#include "utils/logger.hpp"
#include "utils/timer.hpp"
#include "websocket_frame.hpp"
#include "ws_session_base.hpp"

#include <stdint.h>
#include <stddef.h>
#include <string>
#include <memory>
#include <vector>
#include <map>

namespace cpp_streamer
{
class WebSocketServer;
class WebSocketSession : public TcpSessionCallbackI, public WebSocketSessionBase, public TimerInterface
{
public:
    WebSocketSession(uv_loop_t* loop, uv_stream_t* handle, WebSocketServer* server, Logger* logger);
    WebSocketSession(uv_loop_t* loop, uv_stream_t* handle, WebSocketServer* server,
                    const std::string& key_file, const std::string& cert_file, Logger* logger);
    virtual ~WebSocketSession();

public:
    std::string GetRemoteAddress();
    void SetSessionCallback(WebSocketSessionCallBackI* cb);
    int64_t GetLastPongMs();

protected:
    virtual void OnTimer() override;

protected:
    virtual void OnWrite(int ret_code, size_t sent_size) override;
    virtual void OnRead(int ret_code, const char* data, size_t data_size) override;

protected:
    virtual void HandleWsData(uint8_t* data, size_t len, int op_code) override;
    virtual void SendWsFrame(const uint8_t* data, size_t len, uint8_t op_code) override;
    virtual void HandleWsClose(uint8_t* data, size_t len) override;

private:
    void Init();
    int OnHandleHttpRequest();
    void SendHttpResponse();
    void SendErrorResponse();
    void OnHandleFrame(const uint8_t* data, size_t data_size);
    std::string GenHashcode();

private:
    WebSocketServer* server_    = nullptr;
    Logger* logger_             = nullptr;
    std::unique_ptr<TcpSession> session_;

private:
    bool http_request_ready_ = false;
    DataBuffer http_recv_buffer_;

private:
    std::string method_;
    std::string path_;
    std::map<std::string, std::string> headers_;
    int sec_ws_ver_ = 13;
    std::string sec_ws_key_;
    std::string sec_ws_protocol_;

private:
    std::string hash_code_;

private:
    WebSocketFrame frame_;
    std::vector<std::shared_ptr<DataBuffer>> recv_buffer_vec_;
    int die_count_ = 0;

private:
    WebSocketSessionCallBackI* cb_ = nullptr;
};
}

#endif