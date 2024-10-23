#include "websocket_session.hpp"
#include "websocket_server.hpp"
#include "utils/stringex.hpp"
#include "utils/base64.hpp"
#include "utils/byte_crypto.hpp"
#include "utils/timeex.hpp"

namespace cpp_streamer
{

WebSocketSession::WebSocketSession(uv_loop_t* loop, 
                                uv_stream_t* handle, 
                                WebSocketServer* server,
                                Logger* logger):WebSocketSessionBase(logger)
                                            , TimerInterface(loop, 200)
                                            , server_(server)
                                            , logger_(logger)
{
    session_.reset(new TcpSession(loop, handle, this, logger));
    Init();
}

WebSocketSession::WebSocketSession(uv_loop_t* loop, 
                                uv_stream_t* handle,
                                WebSocketServer* server,
                                const std::string& key_file, 
                                const std::string& cert_file, 
                                Logger* logger):WebSocketSessionBase(logger)
                                            , TimerInterface(loop, 200)
                                            , server_(server)
                                            , logger_(logger)
{
    session_.reset(new TcpSession(loop, handle, this, key_file, cert_file, logger));
    Init();
}

WebSocketSession::~WebSocketSession()
{
}

void WebSocketSession::Init() {
    session_->AsyncRead();
    is_connected_ = true;
    last_recv_pong_ms_ = now_millisec();
    StartTimer();
}

void WebSocketSession::OnTimer() {
    if (!is_connected_) {
        return;
    }
    int64_t now_ms = now_millisec();

    if (now_ms - last_send_ping_ms_ > 2000) {
        last_send_ping_ms_ = now_ms;
        SendPingFrame(now_ms);
    }
}

int64_t WebSocketSession::GetLastPongMs() {
    return last_recv_pong_ms_;
}

std::string WebSocketSession::GetRemoteAddress() {
    return session_->GetRemoteEndpoint();
}

void WebSocketSession::SetSessionCallback(WebSocketSessionCallBackI* cb) {
    cb_ = cb;
}

void WebSocketSession::OnWrite(int ret_code, size_t sent_size) {
    if (ret_code < 0) {
        is_connected_ = false;
        LogInfof(logger_, "tcp write return:%d", ret_code);
        return;
    }
}

void WebSocketSession::OnRead(int ret_code, const char* data, size_t data_size) {
    if (ret_code < 0) {
        is_connected_ = false;
        LogInfof(logger_, "tcp read return:%d", ret_code);
        return;
    }

    if (!http_request_ready_) {
        http_recv_buffer_.AppendData(data, data_size);
        try {
            int ret = OnHandleHttpRequest();
            if (ret == 1) {
                session_->AsyncRead();
            } else if (ret == 0) {
                SendHttpResponse();
                http_recv_buffer_.Reset();
                session_->AsyncRead();
            } else {
                SendErrorResponse();
            }
            return;
        } catch(const std::exception& e) {
            is_connected_ = false;
            LogErrorf(logger_, "handle http request(websocket) exception:%s", e.what());
            return;
        }
    }
    DataBuffer recv_data(data_size);
    recv_data.AppendData(data, data_size);
    HandleFrame(recv_data);
    session_->AsyncRead();
}

void WebSocketSession::SendHttpResponse() {
    std::stringstream ss;

    std::string hash_code = GenHashcode();

    ss << "HTTP/1.1 101 Switching Protocols" << "\r\n";
    ss << "Upgrade: websocket" << "\r\n";
    ss << "Connection: Upgrade" << "\r\n";
    ss << "Sec-WebSocket-Accept: " << hash_code << "\r\n";

    ss << "\r\n";

    LogInfof(logger_, "send response:%s", ss.str().c_str());
    session_->AsyncWrite(ss.str().c_str(), ss.str().length());
    return;
}

void WebSocketSession::SendErrorResponse() {
    std::string resp_msg = "HTTP/1.1 400 Bad Request\r\n\r\n";

    LogInfof(logger_, "send error message:%s", resp_msg.c_str());
    session_->AsyncWrite(resp_msg.c_str(), resp_msg.length());
}

int WebSocketSession::OnHandleHttpRequest() {
    std::string content(http_recv_buffer_.Data(), http_recv_buffer_.DataLen());

    size_t pos = content.find("\r\n\r\n");
    if (pos == content.npos) {
        return 1;
    }
    std::vector<std::string> lines;

    http_request_ready_ = true;
    content = content.substr(0, pos);

    int ret = StringSplit(content, "\r\n", lines);
    if (ret <= 0 || lines.empty()) {
        CSM_THROW_ERROR("websocket http header error");
    }
    std::vector<std::string> http_items;
    StringSplit(lines[0], " ", http_items);
    if (http_items.size() != 3) {
        LogErrorf(logger_, "http header error:%s", lines[0].c_str());
        CSM_THROW_ERROR("websocket http header error");
    }
    method_ = http_items[0];
    path_ = http_items[1];

    auto callback_func = server_->GetHandle(path_);
    if (!callback_func) {
        LogErrorf(logger_, "fail to find subpath:%s", path_.c_str());
        CSM_THROW_ERROR("fail to find subpath");
    }

    LogInfof(logger_, "websocket http method:%s", method_.c_str());
    LogInfof(logger_, "websocket http path:%s", path_.c_str());

    String2Lower(method_);
    int index = 0;
    for (auto& line : lines) {
        if (index++ == 0) {
            continue;
        }

        size_t pos = line.find(" ");
        if (pos == line.npos) {
            continue;
        }
        std::string key = line.substr(0, pos - 1);//remove ':'
        std::string value = line.substr(pos + 1);

        String2Lower(key);
        headers_[key] = value;
        LogInfof(logger_, "websocket http header:%s %s", key.c_str(), value.c_str());
    }

    auto connection_iter = headers_.find("connection");
    if (connection_iter == headers_.end()) {
        CSM_THROW_ERROR("websocket http header error: Connection not exist");
    }
    String2Lower(connection_iter->second);
    if (connection_iter->second != "upgrade") {
        LogErrorf(logger_, "http header error:%s %s",
                connection_iter->first.c_str(),
                connection_iter->second.c_str());
        CSM_THROW_ERROR("websocket http header error: Connection is not upgrade");
    }

    auto upgrade_iter = headers_.find("upgrade");
    if (upgrade_iter == headers_.end()) {
        CSM_THROW_ERROR("websocket http header error: Upgrade not exist");
    }
    String2Lower(upgrade_iter->second);
    if (upgrade_iter->second != "websocket") {
        LogErrorf(logger_, "http header error:%s %s",
                connection_iter->first.c_str(),
                connection_iter->second.c_str());
        CSM_THROW_ERROR("websocket http header error: upgrade is not websocket");
    }

    auto ver_iter = headers_.find("sec-websocket-version");
    if (ver_iter != headers_.end()) {
        sec_ws_ver_ = atoi(ver_iter->second.c_str());
    } else {
        sec_ws_ver_ = 13;
    }

    auto key_iter = headers_.find("sec-websocket-key");
    if (key_iter != headers_.end()) {
        sec_ws_key_ = key_iter->second;
    } else {
        CSM_THROW_ERROR("websocket http header error: Sec-WebSocket-Key not exist");
    }

    auto protocal_iter = headers_.find("sec-webSocket-protocol");
    if (protocal_iter != headers_.end()) {
        sec_ws_protocol_ = protocal_iter->second;
    }

    callback_func(path_, this);
    return 0;
}

std::string WebSocketSession::GenHashcode() {
    std::string sec_key = sec_ws_key_;
	sec_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char hash[20];
    SHA_CTX sha1;

    SHA1_Init(&sha1);
    SHA1_Update(&sha1, sec_key.data(), sec_key.size());
    SHA1_Final(hash, &sha1);
	
	hash_code_ = Base64Encode(hash, sizeof(hash));
    return hash_code_;
}

void WebSocketSession::HandleWsData(uint8_t* data, size_t len, int op_code) {
    if (cb_) {
        if (op_code == WS_OP_TEXT_TYPE) {
            cb_->OnReadText(0, std::string((char*)data, len));
        } else if (op_code == WS_OP_BIN_TYPE) {
            cb_->OnReadData(0, data, len);
        } else {
            LogErrorf(logger_, "handle unknown websocket data op_code:%d", op_code);
        }
    }
}

void WebSocketSession::SendWsFrame(const uint8_t* data, size_t len, uint8_t op_code) {
    WS_PACKET_HEADER* ws_header;
    uint8_t header_start[WS_MAX_HEADER_LEN];
    size_t header_len = 2;

    ws_header = (WS_PACKET_HEADER*)header_start;
    memset(header_start, 0, WS_MAX_HEADER_LEN);
    ws_header->fin      = 1;
    ws_header->opcode   = op_code;

    if (len >= 126) {
        if (len > UINT16_MAX) {
            ws_header->payload_len = 127;
			ws_header->payload_len = 127;
			*(uint8_t*)(header_start + 2) = (len >> 56) & 0xFF;
			*(uint8_t*)(header_start + 3) = (len >> 48) & 0xFF;
			*(uint8_t*)(header_start + 4) = (len >> 40) & 0xFF;
			*(uint8_t*)(header_start + 5) = (len >> 32) & 0xFF;
			*(uint8_t*)(header_start + 6) = (len >> 24) & 0xFF;
			*(uint8_t*)(header_start + 7) = (len >> 16) & 0xFF;
			*(uint8_t*)(header_start + 8) = (len >> 8) & 0xFF;
			*(uint8_t*)(header_start + 9) = (len >> 0) & 0xFF;
            header_len = WS_MAX_HEADER_LEN;
        } else {
			ws_header->payload_len = 126;
			*(uint8_t*)(header_start + 2) = (len >> 8) & 0xFF;
			*(uint8_t*)(header_start + 3) = (len >> 0) & 0xFF;
            header_len = 4;
        }
    } else {
        ws_header->payload_len = len;
        header_len = 2;
    }
    ws_header->mask = 1;

    uint8_t masking_key[4];

    masking_key[0] = ByteCrypto::GetRandomUint(1, 0xff);
    masking_key[1] = ByteCrypto::GetRandomUint(1, 0xff);
    masking_key[2] = ByteCrypto::GetRandomUint(1, 0xff);
    masking_key[3] = ByteCrypto::GetRandomUint(1, 0xff);
    
    std::vector<uint8_t> data_vec(len);
    uint8_t* p = &data_vec[0];
    
    memcpy(p, data, len);

    size_t temp_len = len & ~3;
    for (size_t i = 0; i < temp_len; i += 4) {
        p[i + 0] ^= masking_key[0];
        p[i + 1] ^= masking_key[1];
        p[i + 2] ^= masking_key[2];
        p[i + 3] ^= masking_key[3];
    }
    for (size_t i = temp_len; i < len; ++i) {
        p[i] ^= masking_key[i % 4];
    }

    session_->AsyncWrite((char*)header_start, header_len);
    session_->AsyncWrite((char*)masking_key, sizeof(masking_key));
    session_->AsyncWrite((char*)p, len);
}

void WebSocketSession::HandleWsClose(uint8_t* data, size_t len) {
    if (close_) {
        return;
    }

    if (len <= 1) {
        SendClose(1002, "Incomplete close code");
    } else {
		bool invalid = false;
		uint16_t code = (uint8_t(data[0]) << 8) | uint8_t(data[1]);
		if(code < 1000 || code >= 5000) {
            invalid = true;
        }
		
		switch(code){
		    case 1004:
		    case 1005:
		    case 1006:
		    case 1015:
		    	invalid = true;
		    default:;
		}
		
		if(invalid){
			SendClose(1002, "Invalid close code");
		} else {
            SendWsFrame(data, len, WS_OP_CLOSE_TYPE);
        }
    }

    close_ = true;
    session_->Close();
}

}