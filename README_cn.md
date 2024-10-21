中文 | [English](README_.md)
# CPP_WebSocket
websocket客户端和服务器是用c++11开发，基于libuv高性能异步网络库，具有异步高并发的特点。

## websocket服务端
支持高性能的websocket服务

* 基于libuv高性能的网络异步并发
* 支持websocket bin/text数据的收发
* 支持基于http(不加密)的websocket
* 支持基于https(加密)的websocket
* 支持后续websocket的定制开发

这里有服务端的demo: [ws_server_demo](src/tools/ws_server_demo.cpp)

## websocket客户端
支持websocket客户端各种常用功能
* 基于libuv高性能的网络异步并发
* 支持libuv线程内的数据传入
* 支持libuv线程外的数据传入
* 支持基于http(不加密)的websocket
* 支持基于https(加密)的websocket
* 支持后续websocket的定制开发

这里有客户端的demo: 
* [ws_client_demo](src/tools/ws_client_demo.cpp): libuv内部线程发送数据的demo。
* [ws_client_demo](src/tools/ws_client_asnyc_demo.cpp): libuv外部线程发送数据的demo。

## 如何编译(linux/macos)
编译采用cmake，需要cmake 3.7.1以上
```
mkdir objs
cd objs
cmake ..
make
```