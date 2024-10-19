set(CMAKE_OUTPUT_BASE ${CMAKE_BINARY_DIR}/output)
set(BUILD_OUTPUT_BASE ${CMAKE_BINARY_DIR}/output)
set(PREFIX_DIR "${BUILD_OUTPUT_BASE}")
set(INSTALL_RPATH "${PREFIX_DIR}/lib")

SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/objs)

################################################################
# example: websocket client async demo
# the text/data sending operator is not happen in the uv loop thread.
add_executable(ws_client_async_demo
            ${PROJECT_SOURCE_DIR}/src/net/http/http_client.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_client.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/ws_session_base.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_frame.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_pub.cpp
            ${PROJECT_SOURCE_DIR}/src/utils/byte_crypto.cpp
            ${PROJECT_SOURCE_DIR}/src/utils/base64.cpp
            ${PROJECT_SOURCE_DIR}/src/tools/ws_client_demo.cpp)
add_dependencies(ws_client_async_demo uv openssl)
IF (APPLE)
target_link_libraries(ws_client_async_demo dl z m ssl crypto pthread uv)
ELSEIF (UNIX)
target_link_libraries(ws_client_async_demo rt dl z m ssl crypto pthread uv)
ENDIF ()

################################################################
# example: websocket client demo
add_executable(ws_client_demo
            ${PROJECT_SOURCE_DIR}/src/net/http/http_client.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_client.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/ws_session_base.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_frame.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_pub.cpp
            ${PROJECT_SOURCE_DIR}/src/utils/byte_crypto.cpp
            ${PROJECT_SOURCE_DIR}/src/utils/base64.cpp
            ${PROJECT_SOURCE_DIR}/src/tools/ws_client_demo.cpp)
add_dependencies(ws_client_demo uv openssl)
IF (APPLE)
target_link_libraries(ws_client_demo dl z m ssl crypto pthread uv)
ELSEIF (UNIX)
target_link_libraries(ws_client_demo rt dl z m ssl crypto pthread uv)
ENDIF ()

################################################################
# example: websocket server demo
add_executable(ws_server_demo
            ${PROJECT_SOURCE_DIR}/src/net/http/http_client.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_server.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_session.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/ws_session_base.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_frame.cpp
            ${PROJECT_SOURCE_DIR}/src/net/http/websocket/websocket_pub.cpp
            ${PROJECT_SOURCE_DIR}/src/utils/byte_crypto.cpp
            ${PROJECT_SOURCE_DIR}/src/utils/base64.cpp
            ${PROJECT_SOURCE_DIR}/src/tools/ws_server_demo.cpp)
add_dependencies(ws_server_demo uv openssl)
IF (APPLE)
target_link_libraries(ws_server_demo dl z m ssl crypto pthread uv)
ELSEIF (UNIX)
target_link_libraries(ws_server_demo rt dl z m ssl crypto pthread uv)
ENDIF ()