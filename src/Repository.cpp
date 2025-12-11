#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

#include "Commit.hpp"
#include "Repository.h"
#include "Serialization.hpp"
#include "Utils.h"

using std::optional;
using std::string;
using std::string_view;
namespace fs = std::filesystem;

const fs::path Repo::gitDir = ".gitlite";
const fs::path Repo::objDir = ".gitlite/objects";
const fs::path Repo::branchDir = ".gitlite/refs/heads";
const fs::path Repo::headFile = ".gitlite/HEAD";

inline fs::path Repo::id_to_dir(string_view id) {
    return objDir / id.substr(0, 2) / id.substr(2, 38);
}

void Repo::add_commit(const Commit& comm) {
    string id = comm.id;
    ser::serialize_to_file(comm, id_to_dir(id));
}

void Repo::update_branch(string_view branch, string_view comm_id) {
    ser::serialize_to_safe_file(comm_id, branchDir / branch);
}

void Repo::update_head(string_view branch) {
    ser::serialize_to_safe_file(branch, headFile);
}

void Repo::add_init_commit() {
    Commit initial = make_init_commit();
    string id = SHA1::sha1(serialize(initial));
    initial.id = id;
    add_commit(initial);
    update_branch("master", id);
    update_head("master");
    // headCommitId = id;
    // headBranch = "master";
    // branches.emplace("master", id);
}

void Repo::recover_basic_info() {
    ser::deserialize_from_file(headBranch, headFile);
    ser::deserialize_from_file(headCommitId, branchDir / headBranch);
}

void Repo::recover_index() {
    // If index files are missing (fresh repo), keep staging maps empty.
    const auto indexAddPath = gitDir / "INDEX1";
    const auto indexRemovePath = gitDir / "INDEX2";
    if (fs::exists(indexAddPath)) {
        ser::deserialize_from_file(stageAdd, indexAddPath);
    } else {
        stageAdd.clear();
    }
    if (fs::exists(indexRemovePath)) {
        ser::deserialize_from_file(stageRemove, indexRemovePath);
    } else {
        stageRemove.clear();
    }
}

optional<string> Repo::get_id_blob_id(const string& fileName) {
    Commit comm;
    ser::deserialize_from_file(comm, id_to_dir(headCommitId));
    auto it = comm.mapping.find(fileName);
    if (it != comm.mapping.end()) {
        return std::make_optional(it->second);
    }
    return std::nullopt;
}

void Repo::git_add(const string& fileName) {
    // 获取 headCommitId
    recover_basic_info();
    recover_index();

    // 检查文件存在
    if (!fs::exists(fileName)) {
        Utils::exitWithMessage("File does not exist.");
    }

    // 不再删除文件
    stageRemove.erase(fileName);

    // 计算文件对应的哈希
    string content;
    Utils::readContentsAsString(content, fileName);
    string id_in_blob = SHA1::sha1(content);

    // 获取当前 commit 中的哈希并比较
    Commit comm;
    string id_in_commit;
    ser::deserialize_from_file(comm, id_to_dir(headCommitId));
    auto it = comm.mapping.find(fileName);
    if (it != comm.mapping.end() && it->second == id_in_blob) {
        // 如果版本相同，则不添加
        stageAdd.erase(fileName); // fileName 如果本来就不存在，那么就什么都没做
    } else {
        // 添加
        stageAdd[fileName] = id_in_blob;
        // 无论存在但版本不同还是不存在，都记录这个新 blob
        Utils::writeContents(content, id_to_dir(id_in_blob));
    }

    // 写回文件暂存区
    ser::serialize_to_safe_file(stageAdd, gitDir / "INDEX1");
    ser::serialize_to_safe_file(stageRemove, gitDir / "INDEX2");
}

void Repo::git_commit(const string& message) {
    // 错误检查和初始化
    if (message.empty()) {
        Utils::exitWithMessage("Please enter a commit message.");
    }
    recover_index();
    if (stageAdd.empty() && stageRemove.empty()) {
        Utils::exitWithMessage("No changes added to the commit.");
    }
    recover_basic_info();

    Commit old_comm;
    ser::deserialize_from_file(old_comm, id_to_dir(headCommitId));

    // 存入新提交
    Commit comm(message, std::chrono::system_clock::now());
    comm.mapping = std::move(old_comm.mapping);
    comm.parents.emplace_back(old_comm.id);

    for (auto [k, v] : stageAdd) {
        comm.mapping[k] = std::move(v);
    }
    for (const auto& k : stageRemove) {
        comm.mapping.erase(k);
    }

    auto id = SHA1::sha1(serialize(comm));
    comm.id = id;
    ser::serialize_to_file(comm, id_to_dir(id));

    // 清空暂存区
    stageAdd.clear();
    stageRemove.clear();

    ser::serialize_to_safe_file(stageAdd, gitDir / "INDEX1");
    ser::serialize_to_safe_file(stageRemove, gitDir / "INDEX2");

    // 设置分支位置
    update_branch(headBranch, id);
    headCommitId = id;
}

void Repo::git_rm(const string& fileName) {
    // 获取 headCommitId
    recover_basic_info();
    recover_index();

    bool reason = false;

    // 删除文件
    if (stageAdd.erase(fileName) != 0) {
        reason = true;
    };

    // 获取当前 commit 中的哈希并比较
    Commit comm;
    string id_in_commit;
    ser::deserialize_from_file(comm, id_to_dir(headCommitId));
    auto it = comm.mapping.find(fileName);
    if (it != comm.mapping.end()) {
        reason = true;
        stageRemove.emplace(fileName);
        Utils::restrictedDelete(fileName);
    }

    if (!reason) {
        Utils::exitWithMessage("No reason to remove the file.");
    }

    // 写回文件暂存区
    ser::serialize_to_safe_file(stageAdd, gitDir / "INDEX1");
    ser::serialize_to_safe_file(stageRemove, gitDir / "INDEX2");
}

void Repo::init() {
    if (fs::exists(gitDir)) {
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }
    fs::create_directory(gitDir);
    fs::create_directories(objDir); // 仿照 git 的做法，commit 和 blob 放在一起
    fs::create_directories(branchDir);
    fs::create_directories(gitDir / "refs" / "remotes");
    add_init_commit();
}
