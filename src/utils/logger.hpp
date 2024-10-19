#ifndef LOGGER_HPP
#define LOGGER_HPP
#include "timeex.hpp"

#include <string>
#include <stdint.h>
#include <stdarg.h>
#include <cstdio> // std::snprintf()
#include <stdexcept>
#include <assert.h>
#include <stdio.h>
#include <sstream>
#include <iostream>
#include <vector>

namespace cpp_streamer
{

#define LOGGER_BUFFER_SIZE (2*1024*1024)

enum LOGGER_LEVEL {
    LOGGER_DEBUG_LEVEL,
    LOGGER_INFO_LEVEL,
    LOGGER_WARN_LEVEL,
    LOGGER_ERROR_LEVEL
};


class Logger
{
public:
    Logger(const std::string filename = "", enum LOGGER_LEVEL level = LOGGER_INFO_LEVEL):filename_(filename)
    , level_(level)
    {
        buffer_ = new char[buffer_len_];
    }
    ~Logger()
    {
        delete[] buffer_;
        buffer_ = nullptr;
    }

public:
    void SetFilename(const std::string& filename) {
        filename_ = filename;
    }
    void SetLevel(enum LOGGER_LEVEL level) {
        level_ = level;
    }
    void EnableConsole() {
        console_enable_ = true;
    }
    void DisableConsole() {
        console_enable_ = false;
    }
    enum LOGGER_LEVEL GetLevel() {
        return level_;
    }
    void AllocBuffer(size_t len) {
        if (buffer_) {
            delete[] buffer_;
        }
        buffer_ = new char[len];
        buffer_len_ = len;
    }
    char* GetBuffer() {
        return buffer_;
    }
    size_t BufferSize() {
        return buffer_len_;
    }
    void Logf(const char* level, const char* buffer) {
        std::stringstream ss;


        ss << "[" << level << "]" << "[" << get_now_str() << "]"
           << buffer << "\r\n";
        
        if (filename_.empty()) {
            std::cout << ss.str();
        } else {
            FILE* file_p = fopen(filename_.c_str(), "ab+");
            if (file_p) {
                fwrite(ss.str().c_str(), ss.str().length(), 1, file_p);
                fclose(file_p);
            }
            if (console_enable_) {
                std::cout << ss.str();
            }
        }
    }

private:
    std::string filename_;
    enum LOGGER_LEVEL level_;
    char* buffer_ = nullptr;
    size_t buffer_len_ = LOGGER_BUFFER_SIZE;
    bool console_enable_ = false;
};

inline void LogError(Logger* logger, const char* data) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_INFO_LEVEL) {
        return;
    }
    logger->Logf("W", data);
}

inline void LogErrorf(Logger* logger, const char* fmt, ...) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_ERROR_LEVEL) {
        return;
    }
    char* buffer = logger->GetBuffer();
    size_t bsize = logger->BufferSize();
    va_list ap;
 
    va_start(ap, fmt);
    int ret_len = vsnprintf(buffer, bsize, fmt, ap);
    buffer[ret_len] = 0;
    va_end(ap);

    logger->Logf("E", buffer);
}

inline void LogWarn(Logger* logger, const char* data) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_INFO_LEVEL) {
        return;
    }
    logger->Logf("W", data);
}

inline void LogWarnf(Logger* logger, const char* fmt, ...) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_WARN_LEVEL) {
        return;
    }
    char* buffer = logger->GetBuffer();
    size_t bsize = logger->BufferSize();
    va_list ap;
 
    va_start(ap, fmt);
    vsnprintf(buffer, bsize, fmt, ap);
    va_end(ap);

    logger->Logf("W", buffer);
}

inline void LogInfo(Logger* logger, const char* data) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_INFO_LEVEL) {
        return;
    }
    logger->Logf("I", data);
}

inline void LogInfof(Logger* logger, const char* fmt, ...) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_INFO_LEVEL) {
        return;
    }
    char* buffer = logger->GetBuffer();
    size_t bsize = logger->BufferSize();
    va_list ap;
 
    va_start(ap, fmt);
    vsnprintf(buffer, bsize, fmt, ap);
    //int ret_len = vsnprintf(buffer, bsize, fmt, ap);
    //buffer[ret_len] = 0;
    va_end(ap);

    //std::cout << "loginfo size:" << bsize << ", str len:" << ret_len << "\r\n\r\n";
    logger->Logf("I", buffer);
}

inline void LogDebug(Logger* logger, const char* data) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_INFO_LEVEL) {
        return;
    }
    logger->Logf("D", data);
}

inline void LogDebugf(Logger* logger, const char* fmt, ...) {
    if (logger == nullptr || logger->GetLevel() > LOGGER_DEBUG_LEVEL) {
        return;
    }
    char* buffer = logger->GetBuffer();
    size_t bsize = logger->BufferSize();
    va_list ap;
 
    va_start(ap, fmt);
    int ret_len = vsnprintf(buffer, bsize, fmt, ap);
    buffer[ret_len] = 0;
    va_end(ap);

    logger->Logf("D", buffer);
}

inline void LogInfoData(Logger* logger, const uint8_t* data, size_t len, const char* dscr) {
    if (!logger || logger->GetLevel() > LOGGER_INFO_LEVEL) {
        return;
    }
    char print_data[16*1024];
    size_t print_len = 0;
    const int MAX_LINES = 100;
    int line = 0;
    int index = 0;
    print_len += snprintf(print_data, sizeof(print_data), "%s:", dscr);
    for (index = 0; index < (int)len; index++) {
        if ((index%16) == 0) {
            print_len += snprintf(print_data + print_len, sizeof(print_data) - print_len, "\r\n");
            if (++line > MAX_LINES) {
                break;
            }
        }
        print_len += snprintf(print_data + print_len, sizeof(print_data) - print_len,
            " %02x", *(static_cast<const uint8_t*>(data + index)));
    }

    print_data[print_len] = 0;
    logger->Logf("I", print_data);
}

class CppStreamException : public std::exception
{
public:
    explicit CppStreamException(const char* description)
    {
        desc_ = description;
    }

    virtual const char* what() const noexcept { return desc_.c_str(); } 

private:
    std::string desc_;
};

#define CSM_THROW_ERROR(desc, ...) \
    do \
    { \
        char exp_buffer[1024]; \
        int exp_ret_len = std::snprintf(exp_buffer, sizeof(exp_buffer), desc, ##__VA_ARGS__); \
        exp_buffer[exp_ret_len] = 0; \
        throw CppStreamException(exp_buffer); \
    } while (false)

}
#endif //LOGGER_HPP
