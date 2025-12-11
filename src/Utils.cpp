#include "Utils.h"
#include <cstddef>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <cstring>
#include <sys/stat.h>

namespace fs = std::filesystem;

/** Assorted utilities.
 *
 * Give this file a good read as it provides several useful utility functions
 * to save you some time.
 */

// SHA1 implementation
namespace SHA1 {
void SHA::reset() {
    A = 0x67452301;
    B = 0xEFCDAB89;
    C = 0x98BADCFE;
    D = 0x10325476;
    E = 0xC3D2E1F0;
}

std::string SHA::padding(std::string& message) {
    auto originalLength = static_cast<int>(message.length());
    int newLength = ((originalLength + 8) + 63) / 64 * 64;
    std::string newMessage = message;
    newMessage.resize(newLength, 0);
    newMessage[originalLength] = static_cast<char>(0x80);
    int bitLength = originalLength * 8;
    for (int i = newLength - 1; i >= newLength - 8; i--) {
        newMessage[i] = static_cast<char>(bitLength & 0xFF);
        bitLength /= 256;
    }
    return newMessage;
}

SHA::WORD SHA::charToWord(char ch) {
    return (BYTE)ch;
}

SHA::WORD SHA::shiftLeft(WORD x, int n) {
    return (x >> (32 - n)) | (x << n);
}

void SHA::getWord(std::string& message, int index) {
    for (int i = 0; i < 16; i++) {
        Word[i] = (charToWord(message[index + 4 * i]) << 24) + (charToWord(message[index + 4 * i + 1]) << 16) +
                  (charToWord(message[index + 4 * i + 2]) << 8) + charToWord(message[index + 4 * i + 3]);
    }
    for (int i = 16; i < 80; i++) {
        Word[i] = shiftLeft(Word[i - 3] ^ Word[i - 8] ^ Word[i - 14] ^ Word[i - 16], 1);
    }
}

SHA::SHA() : Word(80) {
    reset();
}

SHA::WORD SHA::kt(int t) {
    if (t < 20)
        return 0x5a827999;
    if (t < 40)
        return 0x6ed9eba1;
    if (t < 60)
        return 0x8f1bbcdc;
    return 0xca62c1d6;
}

SHA::WORD SHA::ft(int t, WORD B, WORD C, WORD D) {
    if (t < 20)
        return (B & C) | ((~B) & D);
    if (t < 40)
        return B ^ C ^ D;
    if (t < 60)
        return (B & C) | (B & D) | (C & D);
    return B ^ C ^ D;
}

std::string SHA::sha(std::string message) {
    reset();
    message = padding(message);
    size_t byteLength = message.length();
    for (int i = 0; i < byteLength; i += 64) {
        getWord(message, i);
        WORD a = A;
        WORD b = B;
        WORD c = C;
        WORD d = D;
        WORD e = E;
        for (int j = 0; j < 80; j++) {
            WORD temp = shiftLeft(a, 5) + ft(j, b, c, d) + e + kt(j) + Word[j];
            e = d;
            d = c;
            c = shiftLeft(b, 30);
            b = a;
            a = temp;
        }
        A += a;
        B += b;
        C += c;
        D += d;
        E += e;
    }
    std::stringstream ss;
    ss << std::hex;
    ss << std::setw(8) << std::setfill('0') << A;
    ss << std::setw(8) << std::setfill('0') << B;
    ss << std::setw(8) << std::setfill('0') << C;
    ss << std::setw(8) << std::setfill('0') << D;
    ss << std::setw(8) << std::setfill('0') << E;
    return ss.str();
}

SHA sha;

std::string sha1(std::string message) {
    return sha.sha(std::move(message));
}

std::string sha1(std::string_view s1, std::string_view s2) {
    std::string all;
    all.reserve(s1.size() + s2.size());
    all.append(s1);
    all.append(s2);
    return sha1(all);
}

std::string sha1(std::string_view s1, std::string_view s2, std::string_view s3, std::string_view s4) {
    std::string all;
    all.reserve(s1.size() + s2.size() + s3.size() + s4.size());
    all.append(s1);
    all.append(s2);
    all.append(s3);
    all.append(s4);
    return sha1(all);
}
} // namespace SHA1


/* FILE DELETION */
/** Deletes FILE if it exists and is not a directory.  Returns true
 *  if FILE was deleted, and false otherwise.  Refuses to delete FILE
 *  and throws IllegalArgumentException unless the directory designated by
 *  FILE also contains a directory named .gitlite. */
bool Utils::restrictedDelete(const fs::path& target) {
    fs::path parent = target.has_parent_path() ? target.parent_path() : fs::path(".");
    fs::path gitliteDir = parent / ".gitlite";

    if (!fs::is_directory(gitliteDir)) {
        throw std::invalid_argument("not .gitlite working directory");
    }

    if (fs::is_regular_file(target)) {
        return fs::remove(target);
    }
    return false;
}

/** Print a message composed from MSG and ARGS as for the String.format
 *  method, followed by a newline. */
void Utils::message(const std::string& msg) {
    std::cout << msg << std::endl;
}

void Utils::exitWithMessage(const std::string& msg) {
    message(msg);
    std::exit(0);
}
