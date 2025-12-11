#ifndef GITENGINE_H
#define GITENGINE_H

#include "Repository.h"
#include <string_view>

class GitEngine {
private:
    Repo repo;

public:
    static void init();

    void addRemote(std::string_view name, std::string_view path);
    void rmRemote(std::string_view name);

    void add(const std::string& filename);
    void commit(const std::string& message);
    void rm(std::string_view filename);

    void log();
    void globalLog();
    void find(std::string_view message);
    void status();

    void checkoutBranch(std::string_view branch);
    void checkoutFile(std::string_view filename);
    void checkoutFileInCommit(std::string_view commitId, std::string_view filename);

    void branch(std::string_view name);
    void rmBranch(std::string_view name);
    void reset(std::string_view commitId);
    void merge(std::string_view branch);

    void push(std::string_view remoteName, std::string_view remoteBranch);
    void fetch(std::string_view remoteName, std::string_view remoteBranch);
    void pull(std::string_view remoteName, std::string_view remoteBranch);
};

#endif // GITENGINE_H
