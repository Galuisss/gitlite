#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <filesystem>
#include <map>
#include <string_view>

#include "Commit.hpp"
class Repo {
private:
    static const std::filesystem::path gitDir;
    static const std::filesystem::path objDir;
    static const std::filesystem::path branchDir;
    static const std::filesystem::path headFile;

    std::string headCommitId;                    // 缓存当前 HEAD 提交的 Commit ID
    std::string headBranch;                      // 当前所在的分支名, 不延迟写回
    std::map<std::string, std::string> branches; // 缓存 refs/heads 的内容, 不延迟写回

    static void add_commit(const Commit& comm); // 向 objects 加入提交
    static void add_branch(std::string_view branch, std::string_view comm_id); // 向 refs/heads 加入分支
    static void add_head(std::string_view branch);

    void add_init_commit(); // 向 objects 加入初始提交

public:
    void init(); // 初始化仓库
};

#endif // REPOSITORY_H
