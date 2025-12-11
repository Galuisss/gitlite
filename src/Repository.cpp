#include <filesystem>
#include <string>
#include <string_view>

#include "Commit.hpp"
#include "Repository.h"
#include "Utils.h"
#include "Serialization.hpp"

using std::string;
using std::string_view;
namespace fs = std::filesystem;

const fs::path Repo::gitDir = ".gitlite";
const fs::path Repo::objDir = ".gitlite/objects";
const fs::path Repo::branchDir = ".gitlite/refs/heads";
const fs::path Repo::headFile = ".gitlite/HEAD";

void Repo::add_commit(const Commit& comm) {
    string id = comm.id;
    ser::serialize_to_file(comm, objDir / id.substr(0, 2) / id.substr(2, 38));
}

void Repo::add_branch(string_view branch, string_view comm_id) {
    ser::serialize_to_safe_file(comm_id, branchDir / branch);
}

void Repo::add_head(string_view branch) {
    ser::serialize_to_safe_file(branch, headFile);
}

void Repo::add_init_commit() {
    Commit initial = make_init_commit();
    string id = SHA1::sha1(serialize(initial));
    initial.id = id;
    add_commit(initial);
    add_branch("master", id);
    add_head("master");
    headCommitId = id;
    headBranch = "master";
    branches.emplace("master", id);
}

void Repo::init() {
    if (fs::exists(gitDir)) {
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }
    fs::create_directory(gitDir);
    fs::create_directories(objDir); // 仿照 git 的做法，commit 和 blob 放在一起
    fs::create_directories(branchDir);
    fs::create_directories(gitDir / "refs" / "remotes");
    fs::create_directories(gitDir / "stage");
    add_init_commit();
}
