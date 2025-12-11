#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <dirent.h>
#include <string>
#include <string_view>
#include <filesystem>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace SHA1 {
class SHA {
private:
    using BYTE = uint8_t;
    using WORD = uint32_t;
    WORD A, B, C, D, E;
    std::vector<WORD> Word;
    void reset();
    static std::string padding(std::string& message);
    static inline WORD charToWord(char ch);
    static WORD shiftLeft(WORD x, int n);
    static WORD kt(int t);
    static WORD ft(int t, WORD B, WORD C, WORD D);
    void getWord(std::string& message, int index);

public:
    SHA();
    std::string sha(std::string message);
};
extern SHA sha;
std::string sha1(std::string message);
std::string sha1(std::string_view s1, std::string_view s2);
std::string sha1(std::string_view s1, std::string_view s2, std::string_view s3, std::string_view s4);
} // namespace SHA1

class Utils {
public:
    static const int UID_LENGTH = 40;

    // File operations
    static bool restrictedDelete(const std::filesystem::path& target);

    // Message and error reporting
    static void message(const std::string& msg);
    static void exitWithMessage(const std::string& msg);
};

#endif // UTILS_H
