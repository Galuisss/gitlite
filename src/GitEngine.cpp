#include "GitEngine.h"
#include "Repository.h"
#include <string>

using std::string;
namespace fs = std::filesystem;

void GitEngine::init() {
    repo.init();
}

void GitEngine::add(const string& filename) {
    repo.git_add(filename);
}
