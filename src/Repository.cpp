#include "Repository.h"
#include "Utils.h"

const fs::path Repo::gitDir = ".gitlite";

void Repo::add_init_commit() {}

void Repo::init() {
    if (fs::exists(gitDir)) {
        Utils::exitWithMessage("A Gitlite version-control system already exists in the current directory.");
    }
    fs::create_directory(gitDir);
    fs::create_directories(gitDir / "commits");
    fs::create_directories(gitDir / "blobs");
    fs::create_directories(gitDir / "refs" / "heads");
    fs::create_directories(gitDir / "refs" / "remotes");
    fs::create_directories(gitDir / "stage");
    add_init_commit();
}
