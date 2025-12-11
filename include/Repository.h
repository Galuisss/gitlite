#ifndef REPOSITORY_H
#define REPOSITORY_H

#include <filesystem>
namespace fs = std::filesystem;
class Repo {
private:
    static const fs::path gitDir;
    static void add_init_commit();

public:
    static void init();
};

#endif // REPOSITORY_H
