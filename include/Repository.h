#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <filesystem>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>

#include "Commit.hpp"
class Repo {
    using path = std::filesystem::path;
    using string = std::string;
    using string_view = std::string_view;
    using con_string = const std::string&;

private:
    static const path gitDir;
    static const path objDir;
    static const path branchDir;
    static const path headFile;
    static const path indexFile;
    static const path commitSetFile;
    static const path branchSetFile;

    string headCommitId;               // 当前 HEAD 提交的 Commit ID
    string headBranch;                 // 当前所在的分支名
    std::map<string, string> branches; // refs/heads 的内容
    std::map<string, string> stageAdd; // 暂存区待添加的内容
    std::set<string> stageRemove;      // 暂存区待删除的内容
    std::set<string> allCommits;       // 所有提交的 ID 集合
    std::set<string> allBranches;       // 所有分支的名称集合

    static path id_to_dir(string_view id);
    static void add_commit(const Commit& comm);                         // 向 objects 加入提交
    static void update_branch(string_view branch, string_view comm_id); // 向 refs/heads 写入分支信息
    static void update_head(string_view branch);                        // 向 HEAD 写入头信息

    void add_init_commit(); // 向 objects 加入初始提交

    void recover_basic_info();
    void recover_index();
    void recover_commit_set();
    void persist_commit_set();
    void recover_branch_set();
    void persist_branch_set();

    std::optional<string> get_id_blob_id(const string& fileName);

public:
    void init(); // 初始化仓库
    void git_add(con_string fileName);
    void git_commit(con_string message);
    void git_rm(con_string fileName);
    void git_log();
    void global_log();
    void find(con_string message);
    void checkout_branch(con_string branch);
    void checkout_file(con_string fileName);
    void checkout_file_in_commit(con_string commitId, con_string fileName);
    void status();
    void branch(con_string name);
    void rm_branch(con_string name);
    void reset(con_string commitId);
};

#endif // REPOSITORY_H
