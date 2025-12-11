#pragma once

#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

namespace ser {

template <typename T>
    requires std::is_trivially_copyable_v<T>
void serialize(const T& obj, std::ostream& out) {
    out.write(reinterpret_cast<const char*>(&obj), sizeof(T));
}

template <typename T>
    requires std::is_trivially_copyable_v<T>
void deserialize(T& obj, std::istream& in) {
    in.read(reinterpret_cast<char*>(&obj), sizeof(T));
}

template <typename T>
    requires std::is_trivially_copyable_v<T>
[[nodiscard]] std::string serialize(const T& obj) {
    return std::string(reinterpret_cast<const char*>(&obj), sizeof(T));
}

// string
inline void serialize(const std::string& obj, std::ostream& out) {
    size_t len = obj.size();
    serialize(len, out);
    out.write(obj.data(), static_cast<std::streamsize>(len));
}

inline void deserialize(std::string& obj, std::istream& in) {
    size_t len;
    deserialize(len, in);
    obj.resize(len);
    in.read(obj.data(), static_cast<std::streamsize>(len));
}

[[nodiscard]] inline std::string serialize(const std::string& obj) {
    return obj;
}

// map
template <typename T, typename U> void serialize(const std::map<T, U>& obj, std::ostream& out) {
    size_t len = obj.size();
    serialize(len, out);
    for (auto [k, v] : obj) {
        serialize(k, out);
        serialize(v, out);
    }
}

template <typename K, typename V> void deserialize(std::map<K, V>& obj, std::istream& in) {
    size_t len;
    deserialize(len, in);
    obj.clear();
    for (size_t i = 0; i < len; ++i) {
        K k;
        V v;
        deserialize(k, in);
        deserialize(v, in);
        obj.emplace(std::move(k), std::move(v));
    }
}

template <typename K, typename V> [[nodiscard]] std::string serialize(const std::map<K, V>& obj) {
    std::string all;
    size_t len = obj.size();
    all.append(serialize(len));
    for (auto [k, v] : obj) {
        all.append(serialize(k));
        all.append(serialize(v));
    }
    return all;
}

// vector
template <typename T> void serialize(const std::vector<T>& obj, std::ostream& out) {
    size_t len = obj.size();
    serialize(len, out);
    for (auto i : obj) {
        serialize(i, out);
    }
}

template <typename T> void deserialize(std::vector<T>& obj, std::istream& in) {
    size_t len;
    deserialize(len, in);
    obj.clear();
    obj.reserve(len);
    for (size_t i = 0; i < len; ++i) {
        T t;
        deserialize(t, in);
        obj.push_back(std::move(t));
    }
}

template <typename T> [[nodiscard]] std::string serialize(const std::vector<T>& obj) {
    std::string all;
    for (auto i : obj) {
        all.append(serialize(i));
    }
    return all;
}

// time_point
inline void serialize(const std::chrono::system_clock::time_point& tp, std::ostream& out) {
    std::int64_t secs = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
    serialize(secs, out);
}

inline void deserialize(std::chrono::system_clock::time_point& tp, std::istream& in) {
    std::int64_t secs;
    deserialize(secs, in);
    tp = std::chrono::system_clock::time_point{std::chrono::seconds{secs}};
}

[[nodiscard]] std::string serialize(const std::chrono::system_clock::time_point& tp) {
    std::int64_t secs = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
    return serialize(secs);
}

} // namespace ser
