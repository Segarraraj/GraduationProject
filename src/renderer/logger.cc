#include "renderer/logger.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <ctime>

#define RESET_COLOR "\033[0m\0"
#define DEBUG_COLOR "\033[0;35m\0"
#define ERROR_COLOR "\033[0;31m\0"
#define WARNING_COLOR "\033[0;33m\0"

static const char* header_format = "[%s][%s][%s] ";
static const char* types[] = {"DEBUG", "ERROR", "WARNING"};
static const char* log_file = "log.log";

static void LogHeader(FILE* stream, LogType type, const char* tag) {
  time_t now = time(0);
  tm timestamp = tm();

  localtime_s(&timestamp, &now);

  char time[32] = "\0";

  sprintf_s(time, 32, "%d:%d:%d", timestamp.tm_hour, timestamp.tm_min,
            timestamp.tm_sec);
  fprintf_s(stream, header_format, time, types[type], tag);
}

static void LogConsole(LogType type, const char* tag, const char* log,
                       va_list args) {
  switch (type) {
    case kLogType_Debug:
      printf(DEBUG_COLOR);
      break;
    case kLogType_Error:
      printf(ERROR_COLOR);
      break;
    case kLogType_Warning:
      printf(WARNING_COLOR);
      break;
  }
  LogHeader(stdout, type, tag);
  printf(RESET_COLOR);
  vprintf(log, args);
  printf("\n");
}

void Logger::l(LogType type, const char* tag, const char* log, ...) {
  va_list args;
  va_start(args, 8);
  // TODO: ÑAPA
  int a = va_arg(args, int);
  LogConsole(type, tag, log, args);
  va_end(args);
}

void Logger::e(const char* tag, const char* log, ...) {
  va_list args;
  va_start(args, 8);
  // TODO: ÑAPA
  int a = va_arg(args, int);
  LogConsole(kLogType_Error, tag, log, args);
  va_end(args);
}

void Logger::d(const char* tag, const char* log, ...) {
  va_list args;
  va_start(args, 8);
  // TODO: ÑAPA
  int a = va_arg(args, int);
  LogConsole(kLogType_Debug, tag, log, args);
  va_end(args);
}

void Logger::w(const char* tag, const char* log, ...) {
  va_list args;
  va_start(args, 8);
  // TODO: ÑAPA
  int a = va_arg(args, int);
  LogConsole(kLogType_Warning, tag, log, args);
  va_end(args);
}

Logger::Logger() {}

Logger::~Logger() {}