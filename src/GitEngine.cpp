#include "GitEngine.h"
#include "Repository.h"
#include <string>

using std::string;
namespace fs = std::filesystem;

void GitEngine::init() {
    repo.init();
}

void GitEngine::add(con_string filename) {
    repo.git_add(filename);
}

void GitEngine::commit(con_string message) {
    repo.git_commit(message);
}

void GitEngine::rm(con_string filename) {
    repo.git_rm(filename);
}

void GitEngine::log() {
    repo.git_log();
}

void GitEngine::globalLog() {
    repo.global_log();
}

void GitEngine::find(con_string message) {
    repo.find(message);
}
