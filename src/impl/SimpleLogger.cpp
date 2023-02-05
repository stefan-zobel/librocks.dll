
#include "api/api.h"
#include "impl/SimpleLogger.h"
#include "impl/utils.h"


constexpr size_t MAX_LEN = 64;


// internal helpers' forward declaration
static const std::string datetimeNow();
static inline const char* levelToString(LogLevel level) noexcept;


// static data
LogLevel Logger::maxLogLevel = LogLevel::LOG_INFO;



const std::string getCurrentThreadId() {
    char id_[MAX_LEN] = { 0 };
    sprintf_s(&id_[0], MAX_LEN, "%u", GetCurrentThreadId());
    return std::string(&id_[0]);
}


FILE*& Logger::stream() noexcept {
    static FILE* pStream = nullptr;
    return pStream;
}

void Logger::setStream(FILE* pFile) {
    if (pFile) {
        stream() = pFile;
    }
}

void Logger::setMaxLogLevel(LogLevel level) noexcept {
    Logger::maxLogLevel = level;
}

bool Logger::isInitialized() {
    return (stream() != nullptr);
}

void Logger::initialize() {
    if (!isInitialized()) {
        setStream(openLogfile());
    }
}

Logger& Logger::get(LogLevel level) {
    if (level <= Logger::maxLogLevel /*&& Logger::isInitialized()*/) {
        loggerLogLevel = level;

        const std::string& datetime = datetimeNow();
        const std::string& threadId = getCurrentThreadId();
        str.append(datetime.c_str()).append(" ");
        str.append(levelToString(level));
        str.append(" [").append(threadId.c_str());
        str.append("]\t- ");
    } else {
        loggerLogLevel = static_cast<LogLevel>(static_cast<int>(Logger::maxLogLevel) + 1);
    }
    return *this;
}

Logger::~Logger() {
    if (loggerLogLevel <= Logger::maxLogLevel) {
        FILE* pStream = stream();
        if (pStream) {
            str.append("\n");
            if (fprintf(pStream, "%s", str.c_str()) >= 0) {
                fflush(pStream);
            }
        }
    }
}




inline const char* levelToString(LogLevel level) noexcept {
    static const char* const buffer[] = {"FATAL", "ERROR", "WARN ", "INFO ", "DEBUG", "FINE ", "FINER"};
    return buffer[static_cast<size_t>(level)];
}


static const std::string dateToday()
{
    char buffer[MAX_LEN];
    if (GetDateFormatA(LOCALE_USER_DEFAULT, 0, 0, "yyyy'-'MM'-'dd", &buffer[0], MAX_LEN) == 0) {
        return std::string("Error in dateToday()");
    }
    return std::string(&buffer[0]);
}


static const std::string timeNow()
{
    const char* errTxt = "Error in timeNow()";
    // Number of milliseconds since the system was started
    static DWORD firstTick = GetTickCount();

    char buffer[MAX_LEN];
    if (GetTimeFormatA(LOCALE_USER_DEFAULT, 0, 0, "HH':'mm':'ss", &buffer[0], MAX_LEN) == 0) {
        return std::string(errTxt);
    }

    // this part is nonsense!
    const DWORD currTick = GetTickCount();
    if (currTick < firstTick) {
        // Repair the GetTickCount() overflow after 49.7 days
        firstTick = 0L;
    }

    char result[MAX_LEN] = {0};
    if (sprintf_s(&result[0], MAX_LEN, "%s.%03ld", &buffer[0], static_cast<long>(currTick - firstTick) % 1000) <= 0) {
        return std::string(errTxt);
    }
    return std::string(&result[0]);
}



const std::string datetimeNow()
{
    const std::string& today = dateToday();
    const std::string& now = timeNow();

    std::string datetime(today);
    datetime.append(" ").append(now.c_str());

    return datetime;
}
