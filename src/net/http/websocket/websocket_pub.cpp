#include "websocket_pub.hpp"
#include "utils/base64.hpp"
#include <stdint.h>
#include <string>
#include <openssl/sha.h>

namespace cpp_streamer
{
std::string  GenWebSocketHashcode(const std::string& key) {
    std::string sec_key = key;
	sec_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
	unsigned char hash[20];
    SHA_CTX sha1;

    SHA1_Init(&sha1);
    SHA1_Update(&sha1, sec_key.data(), sec_key.size());
    SHA1_Final(hash, &sha1);
	
	return Base64Encode(hash, sizeof(hash));
}

}