/**
 * See http://drdobbs.com/cpp/201804215?pgno=1 for the general idea.
 */

#pragma once

#include <string>


enum class LogLevel {
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_DEBUG,
    LOG_FINE,
    LOG_FINER
};



#define __LOG_FATAL     Logger().get(LogLevel::LOG_FATAL)
#define __LOG_ERROR     Logger().get(LogLevel::LOG_ERROR)
#define __LOG_WARN      Logger().get(LogLevel::LOG_WARNING)
#define __LOG_INFO      Logger().get(LogLevel::LOG_INFO)
#define __LOG_DEBUG     Logger().get(LogLevel::LOG_DEBUG)
#define __LOG_FINE      Logger().get(LogLevel::LOG_FINE)
#define __LOG_FINER     Logger().get(LogLevel::LOG_FINER)

#define __LARG(arg)     .append(arg)
#define __LARGSZ(arg)   .append(reinterpret_cast<const char*>(arg)) // Beware!



class Logger {
public: // instance methods, constructor & destructor
    Logger();
    ~Logger();

    Logger& get(LogLevel level = LogLevel::LOG_INFO);
    Logger& append(const char*);
    Logger& append(char*);
    Logger& append(const std::string&);
    Logger& append(std::string&);

public: // static methods
    static void setStream(FILE*);
    static void setMaxLogLevel(LogLevel level) noexcept;
    static bool isInitialized();

    inline static bool isLogLevelEnabled(LogLevel level) noexcept {
        return level <= Logger::maxLogLevel;
    }


private:
    Logger(const Logger&); // copy constructor
    Logger& operator= (const Logger&); // assignment operator

private: // static methods
    static FILE*& stream() noexcept;
    static void initialize();

private: // data
    std::string str;
    LogLevel loggerLogLevel;

    static LogLevel maxLogLevel;
};



inline
Logger::Logger()
 : loggerLogLevel(LogLevel::LOG_INFO) {
    initialize();
}

inline
Logger& Logger::append(const char* msg) {
    return append(const_cast<char*>(msg));
}

inline
Logger& Logger::append(char* msg) {
    if (loggerLogLevel <= Logger::maxLogLevel) {
        str.append(msg);
    }
    return *this;
}

inline
Logger& Logger::append(const std::string& msg) {
    return append(const_cast<std::string&>(msg));
}

inline
Logger& Logger::append(std::string& msg) {
    return append(msg.c_str());
}
