#ifndef __GUDE_SERVER_H
#define __GUDE_SERVER_H

#include <map>

#include "iomanager.h"

namespace gude
{

class ServerManager
{
public:
    ServerManager(const std::string& config_path);

    virtual ~ServerManager() {}

    bool init(int argc, char** argv);
    bool run();

public:
    IOManager::ptr getMainIOManager() const { return m_mainIOManager; }

private:
    int main(int argc, char** argv);

protected:
    virtual int main_fiber();

protected:
    int m_argc = 0;
    char** m_argv = nullptr;
    bool m_running = false;
    std::string m_config_path;
    IOManager::ptr m_mainIOManager;
};

}

#endif /*__GUDE_SERVER_H*/
