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

void GitEngine::checkoutBranch(con_string branch) {
    repo.checkout_branch(branch);
}

void GitEngine::checkoutFile(con_string filename) {
    repo.checkout_file(filename);
}
void GitEngine::checkoutFileInCommit(con_string commitId, con_string filename) {
    repo.checkout_file_in_commit(commitId, filename);
}

void GitEngine::status() {
    repo.status();
}

void GitEngine::branch(con_string name) {
    repo.branch(name);
}

void GitEngine::rmBranch(con_string name) {
    repo.rm_branch(name);
}

void GitEngine::reset(con_string commitId) {
    repo.reset(commitId);
}

void GitEngine::merge(con_string branch) {
    repo.merge(branch);
}
