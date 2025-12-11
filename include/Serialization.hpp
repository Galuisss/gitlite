#pragma once

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace ser {

// base case
template <typename T>
    requires(std::is_trivially_copyable_v<T>)
void serialize(const T& obj, std::ostream& out) {
    out.write(reinterpret_cast<const char*>(&obj), sizeof(T));
}

template <typename T>
    requires(std::is_trivially_copyable_v<T>)
void deserialize(T& obj, std::istream& in) {
    in.read(reinterpret_cast<char*>(&obj), sizeof(T));
}

template <typename T>
    requires(std::is_trivially_copyable_v<T>)
[[nodiscard]] std::string serialize(const T& obj) {
    return std::string(reinterpret_cast<const char*>(&obj), sizeof(T));
}

// string
inline void serialize(std::string_view obj, std::ostream& out) {
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

[[nodiscard]] inline std::string serialize(std::string_view obj) {
    std::string all;
    size_t len = obj.size();
    all.append(serialize(len));
    all.append(obj);
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

[[nodiscard]] inline std::string serialize(const std::chrono::system_clock::time_point& tp) {
    std::int64_t secs = std::chrono::duration_cast<std::chrono::seconds>(tp.time_since_epoch()).count();
    return serialize(secs);
}

// map
template <typename K, typename V>
    requires requires(K k, V v) {
        serialize(k, std::declval<std::ostream&>());
        serialize(v, std::declval<std::ostream&>());
    }
void serialize(const std::map<K, V>& obj, std::ostream& out) {
    size_t len = obj.size();
    serialize(len, out);
    for (const auto& [k, v] : obj) {
        serialize(k, out);
        serialize(v, out);
    }
}

template <typename K, typename V>
    requires requires(K k, V v) {
        deserialize(k, std::declval<std::istream&>());
        deserialize(v, std::declval<std::istream&>());
    }
void deserialize(std::map<K, V>& obj, std::istream& in) {
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

template <typename K, typename V>
    requires requires(K k, V v) {
        serialize(k);
        serialize(v);
    }
[[nodiscard]] std::string serialize(const std::map<K, V>& obj) {
    std::string all;
    size_t len = obj.size();
    all.append(serialize(len));
    for (const auto& [k, v] : obj) {
        all.append(serialize(k));
        all.append(serialize(v));
    }
    return all;
}

// vector
template <typename T>
    requires requires(T x) { serialize(x, std::declval<std::ostream&>()); }
void serialize(const std::vector<T>& obj, std::ostream& out) {
    size_t len = obj.size();
    serialize(len, out);
    for (const auto& i : obj) {
        serialize(i, out);
    }
}

template <typename T>
    requires requires(T x) { deserialize(x, std::declval<std::istream&>()); }
void deserialize(std::vector<T>& obj, std::istream& in) {
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

template <typename T>
    requires requires(T x) { serialize(x); }
[[nodiscard]] std::string serialize(const std::vector<T>& obj) {
    std::string all;
    size_t len = obj.size();
    all.append(serialize(len));
    for (const auto& i : obj) {
        all.append(serialize(i));
    }
    return all;
}

// set
template <typename T>
    requires requires(T x) { serialize(x, std::declval<std::ostream&>()); }
void serialize(const std::set<T>& obj, std::ostream& out) {
    size_t len = obj.size();
    serialize(len, out);
    for (const auto& i : obj) {
        serialize(i, out);
    }
}

template <typename T>
    requires requires(T x) { deserialize(x, std::declval<std::istream&>()); }
void deserialize(std::set<T>& obj, std::istream& in) {
    size_t len;
    deserialize(len, in);
    obj.clear();
    for (size_t i = 0; i < len; ++i) {
        T t;
        deserialize(t, in);
        obj.insert(std::move(t));
    }
}

template <typename T>
    requires requires(T x) { serialize(x); }
[[nodiscard]] std::string serialize(const std::set<T>& obj) {
    std::string all;
    size_t len = obj.size();
    all.append(serialize(len));
    for (const auto& i : obj) {
        all.append(serialize(i));
    }
    return all;
}

// file operations about serialization
template <typename T>
    requires requires(T x) { serialize(x, std::declval<std::ostream&>()); }
void serialize_to_file(const T& obj, const std::filesystem::path& target) {
    auto parent = target.parent_path();
    std::filesystem::create_directories(parent);

    std::ofstream file(target, std::ios::binary);
    if (!file.is_open()) {
        throw std::invalid_argument("cannot create file");
    }
    serialize(obj, file);
}

template <typename T>
    requires requires(T x) { serialize(x, std::declval<std::ostream&>()); }
void serialize_to_safe_file(const T& obj, const std::filesystem::path& target) {
    std::ofstream file(target, std::ios::binary);
    if (!file.is_open()) {
        throw std::invalid_argument("cannot open file");
    }
    serialize(obj, file);
}

template <typename T>
    requires requires(T x) { deserialize(x, std::declval<std::istream&>()); }
void deserialize_from_file(T& obj, const std::filesystem::path& target) {
    std::ifstream file(target, std::ios::binary);
    if (!file.is_open()) {
        throw std::invalid_argument("cannot open file");
    }
    deserialize(obj, file);
}

} // namespace ser
