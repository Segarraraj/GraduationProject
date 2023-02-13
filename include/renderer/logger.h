#ifndef __LOGGER_H__
#define __LOGGER_H__ 1

enum LogType { kLogType_Debug = 0, kLogType_Error, kLogType_Warning };

class Logger {
 public:
  static void l(LogType type, const char* tag, const char* log, ...);

  static void e(const char* tag, const char* log, ...);
  static void d(const char* tag, const char* log, ...);
  static void w(const char* tag, const char* log, ...);

 private:
  Logger();
  ~Logger();
};

// Global logger macros
#ifdef VERBOSE
#define LOG(type, tag, msg, ...) Logger::l(type, msg, __VA_ARGS__)
#define LOG_WARNING(tag, msg, ...) Logger::w(tag, msg, __VA_ARGS__)
#define LOG_ERROR(tag, msg, ...) Logger::e(tag, msg, __VA_ARGS__)
#define LOG_DEBUG(tag, msg, ...) Logger::d(tag, msg, __VA_ARGS__)
#else
#define LOG(type, tag, msg, ...)
#define LOG_WARNING(msg, tag, ...)
#define LOG_ERROR(msg, tag, ...)
#define LOG_DEBUG(msg, tag, ...)
#endif

#endif  // !__LOGGER_H__
