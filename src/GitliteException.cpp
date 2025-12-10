#include "../include/GitliteException.h"

GitliteException::GitliteException() = default;

GitliteException::GitliteException(const std::string& msg) : message(msg) {}

const char* GitliteException::what() const noexcept {
    return message.c_str();
}
