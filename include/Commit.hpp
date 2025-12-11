#ifndef COMMIT_H
#define COMMIT_H

#include "Serialization.hpp"
#include <chrono>
#include <map>
#include <string>
#include <vector>

struct Commit {
    std::string id;
    std::string message;
    std::vector<std::string> parents;
    std::chrono::system_clock::time_point timestamp;
    std::map<std::string, std::string> mapping;
    Commit() = default;
    explicit Commit(std::string _message, std::chrono::system_clock::time_point _timestamp)
        : message(std::move(_message)), timestamp(_timestamp) {}
};

[[nodiscard]] inline Commit make_init_commit() {
    return Commit("initial commit", std::chrono::system_clock::time_point{});
}

inline void serialize(const Commit& obj, std::ostream& out) {
    ser::serialize(obj.id, out);
    ser::serialize(obj.message, out);
    ser::serialize(obj.parents, out);
    ser::serialize(obj.timestamp, out);
    ser::serialize(obj.mapping, out);
}

inline void deserialize(Commit& obj, std::istream& in) {
    ser::deserialize(obj.id, in);
    ser::deserialize(obj.message, in);
    ser::deserialize(obj.parents, in);
    ser::deserialize(obj.timestamp, in);
    ser::deserialize(obj.mapping, in);
}

[[nodiscard]] inline std::string serialize(const Commit& obj) {
    std::string all;
    all.append(ser::serialize(obj.message));
    all.append(ser::serialize(obj.parents));
    all.append(ser::serialize(obj.timestamp));
    all.append(ser::serialize(obj.mapping));
    return all;
}

#endif // COMMIT_H
