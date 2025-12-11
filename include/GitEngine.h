#ifndef GITENGINE_H
#define GITENGINE_H

#include "Repository.h"

class GitEngine {
    using con_string = const std::string&;

private:
    Repo repo;

public:
    void init();

    void addRemote(con_string name, con_string path);
    void rmRemote(con_string name);

    void add(con_string filename);
    void commit(con_string message);
    void rm(con_string filename);

    void log();
    void globalLog();
    void find(con_string message);
    void status();

    void checkoutBranch(con_string branch);
    void checkoutFile(con_string filename);
    void checkoutFileInCommit(con_string commitId, con_string filename);

    void branch(con_string name);
    void rmBranch(con_string name);
    void reset(con_string commitId);
    void merge(con_string branch);

    void push(con_string remoteName, con_string remoteBranch);
    void fetch(con_string remoteName, con_string remoteBranch);
    void pull(con_string remoteName, con_string remoteBranch);
};

#endif // GITENGINE_H
