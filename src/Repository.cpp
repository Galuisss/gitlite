#include <chrono>
#include <filesystem>
#include <format>
#include <iostream>
#include <optional>
#include <queue>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "Commit.hpp"
#include "Repository.h"
#include "Serialization.hpp"
#include "Utils.h"

using std::cout;
using std::format;
using std::optional;
using std::string;
using std::string_view;
using std::vector;

namespace fs = std::filesystem;

const fs::path Repo::gitDir = ".gitlite";
const fs::path Repo::objDir = ".gitlite/objects";
const fs::path Repo::branchDir = ".gitlite/refs/heads";
const fs::path Repo::headFile = ".gitlite/HEAD";
const fs::path Repo::commitSetFile = ".gitlite/COMMITS";
const fs::path Repo::branchSetFile = ".gitlite/BRANCHES";

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
    allBranches.emplace("master");
    update_branch("master", id);
    update_head("master");
    // headCommitId = id;
    // headBranch = "master";
    // branches.emplace("master", id);
    allCommits.insert(id);
    persist_commit_set();
    persist_branch_set();
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

void Repo::recover_commit_set() {
    if (fs::exists(commitSetFile)) {
        ser::deserialize_from_file(allCommits, commitSetFile);
    } else {
        allCommits.clear();
    }
}

void Repo::persist_commit_set() {
    ser::serialize_to_safe_file(allCommits, commitSetFile);
}

void Repo::recover_branch_set() {
    if (fs::exists(branchSetFile)) {
        ser::deserialize_from_file(allBranches, branchSetFile);
    } else {
        allBranches.clear();
    }
}

void Repo::persist_branch_set() {
    ser::serialize_to_safe_file(allBranches, branchSetFile);
}

optional<string> Repo::get_id_blob_id(con_string fileName) {
    Commit comm;
    ser::deserialize_from_file(comm, id_to_dir(headCommitId));
    auto it = comm.mapping.find(fileName);
    if (it != comm.mapping.end()) {
        return std::make_optional(it->second);
    }
    return std::nullopt;
}

void Repo::git_add(con_string fileName) {
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

void Repo::git_commit(con_string message) {
    // 错误检查和初始化
    if (message.empty()) {
        Utils::exitWithMessage("Please enter a commit message.");
    }
    recover_index();
    if (stageAdd.empty() && stageRemove.empty()) {
        Utils::exitWithMessage("No changes added to the commit.");
    }
    recover_basic_info();
    recover_commit_set();

    Commit old_comm;
    ser::deserialize_from_file(old_comm, id_to_dir(headCommitId));

    // 存入新提交
    Commit comm(message, std::chrono::system_clock::now());
    comm.mapping = std::move(old_comm.mapping);
    comm.parents.emplace_back(old_comm.id);

    for (auto&& [k, v] : stageAdd) {
        comm.mapping[k] = std::move(v);
    }
    for (const auto& k : stageRemove) {
        comm.mapping.erase(k);
    }

    auto id = SHA1::sha1(serialize(comm));
    comm.id = id;
    ser::serialize_to_file(comm, id_to_dir(id));

    allCommits.insert(id);

    // 清空暂存区
    stageAdd.clear();
    stageRemove.clear();

    ser::serialize_to_safe_file(stageAdd, gitDir / "INDEX1");
    ser::serialize_to_safe_file(stageRemove, gitDir / "INDEX2");

    // 设置分支位置
    update_branch(headBranch, id);
    headCommitId = id;
    persist_commit_set();
}

void Repo::git_rm(con_string fileName) {
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

[[nodiscard]] string format_time_point(const std::chrono::system_clock::time_point& tp) {
    using namespace std::chrono;

    // Gitlet tests expect second precision; strip sub-second digits.
    zoned_time zt{current_zone(), floor<seconds>(tp)};

    return std::format("Date: {:%a %b %d %H:%M:%S %Y %z}", zt);
}

inline void print_commit(const Commit& comm) {
    cout << "===\n";
    cout << format("commit {}\n", comm.id);
    if (comm.parents.size() >= 2) {
        cout << format("Merge: {} {}\n", comm.parents[0].substr(0, 7), comm.parents[1].substr(0, 7));
    }
    cout << format_time_point(comm.timestamp) << "\n";
    cout << comm.message << "\n\n";
}
void Repo::git_log() {
    recover_basic_info();
    Commit comm;
    ser::deserialize_from_file(comm, id_to_dir(headCommitId));

    while (true) {
        print_commit(comm);
        if (comm.parents.empty())
            break;
        ser::deserialize_from_file(comm, id_to_dir(comm.parents[0]));
    }
}

void Repo::global_log() {
    recover_commit_set();
    Commit comm;
    for (const auto& id : allCommits) {
        ser::deserialize_from_file(comm, id_to_dir(id));
        print_commit(comm);
    }
}

void Repo::find(con_string message) {
    recover_commit_set();
    Commit comm;
    bool non_empty = false;
    for (const auto& id : allCommits) {
        ser::deserialize_from_file(comm, id_to_dir(id));
        if (comm.message == message) {
            non_empty = true;
            cout << comm.id << '\n';
        }
    }
    if (!non_empty) {
        Utils::exitWithMessage("Found no commit with that message.");
    }
}

void Repo::checkout_file(con_string fileName) {
    recover_basic_info();
    Commit comm;
    ser::deserialize_from_file(comm, id_to_dir(headCommitId));
    auto it = comm.mapping.find(fileName);
    if (it != comm.mapping.end()) {
        fs::copy_file(id_to_dir(it->second), fileName, fs::copy_options::overwrite_existing);
    } else {
        Utils::exitWithMessage("File does not exist in that commit.");
    }
}

void Repo::checkout_file_in_commit(con_string commitId, con_string fileName) {
    recover_commit_set();
    auto it = allCommits.lower_bound(commitId);
    if (it == allCommits.end() || it->compare(0, commitId.size(), commitId) != 0) {
        Utils::exitWithMessage("No commit with that id exists.");
    }

    Commit comm;
    ser::deserialize_from_file(comm, id_to_dir(*it));
    auto it2 = comm.mapping.find(fileName);
    if (it2 != comm.mapping.end()) {
        fs::copy_file(id_to_dir(it2->second), fileName, fs::copy_options::overwrite_existing);
    } else {
        Utils::exitWithMessage("File does not exist in that commit.");
    }
}

void Repo::checkout_branch(con_string branch) {
    recover_basic_info();
    // 该分支是当前分支
    if (branch == headBranch) {
        Utils::exitWithMessage("No need to checkout the current branch.");
    }

    recover_branch_set();
    // 不存在同名分支
    if (!allBranches.contains(branch)) {
        Utils::exitWithMessage("No such branch exists.");
    }

    const fs::path p = ".";
    Commit src;
    ser::deserialize_from_file(src, id_to_dir(headCommitId));
    Commit dst;
    string id;
    ser::deserialize_from_file(id, branchDir / branch);
    ser::deserialize_from_file(dst, id_to_dir(id));

    for (const auto& entry : fs::directory_iterator(p)) {
        auto name = entry.path().filename().string();
        if (!src.mapping.contains(name) && dst.mapping.contains(name)) {
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }

    // 删除当前提交有但目标提交没有的跟踪文件
    for (const auto& [name, _] : src.mapping) {
        if (!dst.mapping.contains(name)) {
            Utils::restrictedDelete(p / name);
        }
    }

    // 将目标提交中的文件写入工作区
    for (const auto& [name, blobId] : dst.mapping) {
        fs::copy_file(id_to_dir(blobId), p / name, fs::copy_options::overwrite_existing);
    }

    // 切换分支并清空暂存区
    headBranch = branch;
    headCommitId = id;
    stageAdd.clear();
    stageRemove.clear();
    ser::serialize_to_safe_file(stageAdd, gitDir / "INDEX1");
    ser::serialize_to_safe_file(stageRemove, gitDir / "INDEX2");

    update_head(branch);
}

void Repo::status() {
    recover_basic_info();
    recover_branch_set();
    recover_index();

    cout << "=== Branches ===\n";
    cout << format("*{}\n", headBranch);
    for (const auto& i : allBranches) {
        if (i != headBranch) {
            cout << i << '\n';
        }
    }
    cout << "\n=== Staged Files ===\n";
    for (const auto& i : stageAdd) {
        cout << i.first << '\n';
    }
    cout << "\n=== Removed Files ===\n";
    for (const auto& i : stageRemove) {
        cout << i << '\n';
    }
    cout << "\n=== Modifications Not Staged For Commit ===\n";
    cout << "\n=== Untracked Files ===\n";
}

void Repo::branch(con_string name) {
    recover_basic_info();
    recover_branch_set();
    if (allBranches.contains(name)) {
        Utils::exitWithMessage("A branch with that name already exists.");
    }
    string id = headCommitId;
    update_branch(name, id);
    allBranches.emplace(name);
    persist_branch_set();
}

void Repo::rm_branch(con_string name) {
    recover_basic_info();
    if (headBranch == name) {
        Utils::exitWithMessage("Cannot remove the current branch.");
    }

    recover_branch_set();
    if (!allBranches.contains(name)) {
        Utils::exitWithMessage("A branch with that name does not exist.");
    }

    // 删除分支引用文件（不使用 restrictedDelete，避免路径检查失效）
    fs::remove(branchDir / name);
    allBranches.erase(name);
    persist_branch_set();
}

void Repo::reset(con_string commitId) {
    recover_commit_set();
    if (!allCommits.contains(commitId)) {
        Utils::exitWithMessage("No commit with that id exists.");
    }
    recover_basic_info();

    const fs::path p = ".";
    Commit src;
    ser::deserialize_from_file(src, id_to_dir(headCommitId));
    Commit dst;
    ser::deserialize_from_file(dst, id_to_dir(commitId));

    for (const auto& entry : fs::directory_iterator(p)) {
        auto name = entry.path().filename().string();
        if (!src.mapping.contains(name) && dst.mapping.contains(name)) {
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }
    }

    // 删除当前提交有但目标提交没有的跟踪文件
    for (const auto& [name, _] : src.mapping) {
        if (!dst.mapping.contains(name)) {
            Utils::restrictedDelete(p / name);
        }
    }

    // 将目标提交中的文件写入工作区
    for (const auto& [name, blobId] : dst.mapping) {
        fs::copy_file(id_to_dir(blobId), p / name, fs::copy_options::overwrite_existing);
    }

    // 切换分支并清空暂存区
    stageAdd.clear();
    stageRemove.clear();
    ser::serialize_to_safe_file(stageAdd, gitDir / "INDEX1");
    ser::serialize_to_safe_file(stageRemove, gitDir / "INDEX2");

    update_branch(headBranch, commitId);
}

[[nodiscard]] Commit Repo::merge_base(Commit A, Commit B) {
    std::queue<Commit> q1;
    std::queue<Commit> q2;
    q1.push(std::move(A));
    q2.push(std::move(B));

    std::unordered_map<string, int> color;
    Commit ans;
    bool flag = true;
    while (flag) {
        size_t s1 = q1.size();
        for (int i = 0; i < s1 && flag; ++i) {
            auto f = q1.front();
            q1.pop();
            auto it = color.find(f.id);
            if (it != color.end() && it->second == 2) {
                ans = f;
                flag = false;
                break;
            };
            color[f.id] = 1;
            for (const auto& p : f.parents) {
                Commit parent;
                ser::deserialize_from_file(parent, id_to_dir(p));
                q1.push(std::move(parent));
            }
        }
        size_t s2 = q2.size();
        for (int i = 0; i < s2 && flag; ++i) {
            auto f = q2.front();
            q2.pop();
            auto it = color.find(f.id);
            if (it != color.end() && it->second == 1) {
                ans = f;
                flag = false;
                break;
            };
            color[f.id] = 2;
            for (const auto& p : f.parents) {
                Commit parent;
                ser::deserialize_from_file(parent, id_to_dir(p));
                q2.push(std::move(parent));
            }
        }
    }
    return ans;
}

void Repo::merge(con_string branch) {
    if (!fs::exists(branchDir / branch)) {
        Utils::exitWithMessage("A branch with that name does not exist.");
    }
    recover_basic_info();
    if (branch == headBranch) {
        Utils::exitWithMessage("Cannot merge a branch with itself.");
    }
    recover_index();
    if (!stageAdd.empty() || !stageRemove.empty()) {
        Utils::exitWithMessage("You have uncommitted changes.");
    }
    string commit_a = headCommitId;
    string commit_b;
    ser::deserialize_from_file(commit_b, branchDir / branch);
    Commit A;
    Commit B;
    ser::deserialize_from_file(A, id_to_dir(commit_a));
    ser::deserialize_from_file(B, id_to_dir(commit_b));

    Commit base = merge_base(A, B);
    string base_id = base.id;
    if (base_id == commit_a) {
        reset(commit_b);
        Utils::exitWithMessage("Current branch fast-forwarded.");
    }
    if (base_id == commit_b) {
        Utils::exitWithMessage("Given branch is an ancestor of the current branch.");
    }

    auto& map_a = A.mapping;
    auto& map_b = B.mapping;
    auto& map_c = base.mapping;
    bool conflict = false;
    for (const auto& [k, vbase] : map_c) {
        auto itA = map_a.find(k);
        auto itB = map_b.find(k);
        bool deletedA = itA == map_a.end();
        bool changedA = deletedA || itA->second != vbase;
        bool deletedB = itB == map_b.end();
        bool changedB = deletedB || itB->second != vbase;
        // 1+6. 在给定分支中变动，本地没变动，听对方的！
        if (changedB && !changedA) {
            // 未跟踪覆盖检查
            if (fs::exists(k) && !map_a.contains(k)) {
                Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
            }
            // 6. 给定分支删除
            if (deletedB) {
                Utils::restrictedDelete(k);
                stageRemove.insert(k);
            }
            // 1. 给定分支修改（非删除）
            else {
                fs::copy_file(id_to_dir(itB->second), k, fs::copy_options::overwrite_existing);
                stageAdd[k] = itB->second;
            }
        }
        // 2+3+7. 本地变动，对方不变动或者变动一致，听本地的！
        // 2. 在当前分支中已修改，对方未修改 / 3. 两者改动方式相同 / 7. 当前分支中已删除，对方未修改
        else if (changedA && !changedB || (changedA && changedB && (deletedA || itA->second == itB->second))) {
            continue;
        }
        // 情况 8 的一种: 变动方式不同
        else if (changedA && changedB) {
            // 未跟踪覆盖检查
            if (!map_a.contains(k) && fs::exists(k) && !stageAdd.contains(k) && !stageRemove.contains(k)) {
                Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
            }
            conflict = true;
            string all = "<<<<<<< HEAD\n";
            if (!deletedA) {
                string contentA;
                Utils::readContentsAsString(contentA, id_to_dir(itA->second));
                while (!contentA.empty() && (contentA.back() == '\n' || contentA.back() == '\r')) {
                    contentA.pop_back();
                }
                all.append(contentA);
            }
            all.append("\n=======\n");
            if (!deletedB) {
                string contentB;
                Utils::readContentsAsString(contentB, id_to_dir(itB->second));
                while (!contentB.empty() && (contentB.back() == '\n' || contentB.back() == '\r')) {
                    contentB.pop_back();
                }
                all.append(contentB);
            }
            if (!deletedB) {
                all.append("\n>>>>>>>\n");
            } else {
                all.append(">>>>>>>\n");
            }
            Utils::writeContents_safe(all, k);
            string blobId = SHA1::sha1(all);
            Utils::writeContents(all, id_to_dir(blobId));
            stageAdd[k] = blobId;
            stageRemove.erase(k);
        }
    }

    // 情况 5
    for (const auto& [k, blobB] : map_b) {
        if (map_c.contains(k) || map_a.contains(k))
            continue;

        // 未跟踪覆盖检查
        fs::path path = k;
        if (fs::exists(path) && !stageAdd.contains(k) && !stageRemove.contains(k) && !map_a.contains(k)) {
            Utils::exitWithMessage("There is an untracked file in the way; delete it, or add and commit it first.");
        }

        // 取目标版本并暂存
        fs::copy_file(id_to_dir(blobB), path, fs::copy_options::overwrite_existing);
        stageAdd[k] = blobB;
    }

    // 情况 8 的第二种：分割点都没有，现在都有且不同
    for (const auto& [k, blobA] : map_a) {
        if (map_c.contains(k) || !map_b.contains(k))
            continue;
        if (blobA == map_b.find(k)->second)
            continue;
        conflict = true;
        string all = "<<<<<<< HEAD\n";
        string contentA;
        Utils::readContentsAsString(contentA, id_to_dir(blobA));
        while (!contentA.empty() && (contentA.back() == '\n' || contentA.back() == '\r')) {
            contentA.pop_back();
        }
        all.append(contentA);
        all.append("\n=======\n");
        string contentB;
        Utils::readContentsAsString(contentB, id_to_dir(map_b.find(k)->second));
        while (!contentB.empty() && (contentB.back() == '\n' || contentB.back() == '\r')) {
            contentB.pop_back();
        }
        all.append(contentB);
        if (!contentB.empty()) {
            all.append("\n>>>>>>>\n");
        } else {
            all.append(">>>>>>>\n");
        }
        Utils::writeContents_safe(all, k);
        string blobId = SHA1::sha1(all);
        Utils::writeContents(all, id_to_dir(blobId));
        stageAdd[k] = blobId;
        stageRemove.erase(k);
    }

    // 写回暂存区
    ser::serialize_to_safe_file(stageAdd, gitDir / "INDEX1");
    ser::serialize_to_safe_file(stageRemove, gitDir / "INDEX2");

    Commit comm(format("Merged {} into {}.", branch, headBranch), std::chrono::system_clock::now());
    comm.parents.emplace_back(A.id);
    comm.parents.emplace_back(B.id);
    comm.mapping = map_a;
    for (auto&& [k, v] : stageAdd) {
        comm.mapping[k] = std::move(v);
    }
    for (const auto& k : stageRemove) {
        comm.mapping.erase(k);
    }

    auto id = SHA1::sha1(serialize(comm));
    comm.id = id;
    ser::serialize_to_file(comm, id_to_dir(id));

    allCommits.insert(id);

    // 清空暂存区
    stageAdd.clear();
    stageRemove.clear();
    ser::serialize_to_safe_file(stageAdd, gitDir / "INDEX1");
    ser::serialize_to_safe_file(stageRemove, gitDir / "INDEX2");

    // 设置分支位置
    update_branch(headBranch, id);
    headCommitId = id;
    persist_commit_set();

    if (conflict) {
        Utils::message("Encountered a merge conflict.");
    }
}
