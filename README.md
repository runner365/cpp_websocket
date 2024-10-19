English | [中文](README_cn.md)
# CPP_WebSocket
The websocket client and server are developed in c++11, based on the libuv high-performance asynchronous network library, and have the characteristics of asynchronous high concurrency.

## websocket server
Support high-performance websocket service

* Based on libuv high-performance network asynchronous concurrency
* Support websocket bin/text data sending and receiving
* Support websocket based on http (unencrypted)
* Support websocket based on https (encrypted)
* Support subsequent websocket custom development

Here is a server demo: [ws_server_demo](src/tools/ws_server_demo.cpp)

## websocket client
Support various common functions of websocket client
* Based on libuv high-performance network asynchronous concurrency
* Support data input within libuv thread
* Support data input outside libuv thread
* Support websocket based on http (unencrypted)
* Support websocket based on https (encrypted)
* Support subsequent websocket custom development

Here are the client demos:
* [ws_client_demo](src/tools/ws_client_demo.cpp): A demo of libuv internal thread sending data.
* [ws_client_demo](src/tools/ws_client_asnyc_demo.cpp): A demo of libuv external thread sending data.