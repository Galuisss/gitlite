#include "GitEngine.h"
#include "Repository.h"
#include <string>

using std::string;
namespace fs = std::filesystem;

void GitEngine::init() {
    Repo::init();
}

void GitEngine::add(const string& filename) {
    repo.git_add(filename);
}

void GitEngine::commit(const string& message) {
    repo.git_commit(message);
}

void GitEngine::rm(const string& filename) {
    repo.git_rm(filename);
}
