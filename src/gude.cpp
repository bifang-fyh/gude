#include "bifang.h"
#include "dht_server.h"

namespace gude
{

SystemLogger();

class GudeServer : public bifang::ServerManager
{
public:
    GudeServer(const std::string& config_path)
        :bifang::ServerManager(config_path)
    {
    }

private:
    virtual int main_fiber() override;
};

// virtual
int GudeServer::main_fiber()
{
    bifang::IOManager* accept = bifang::WorkerMgr::GetInstance()->getAsIOManager("accept").get();
    if (!accept)
    {
        log_error << "accept not exists";
        _exit(0);
    }
    bifang::IOManager* process = bifang::WorkerMgr::GetInstance()->getAsIOManager("process").get();
    if (!process)
    {
        log_error << "process not exists";
        _exit(0);
    }

    bifang::IPv4Address::ptr addr(new bifang::IPv4Address(INADDR_ANY, 6881));
    DhtServer::ptr server(new DhtServer(accept, process));
    if (!server->bind(addr))
    {
        log_error << "bind address fail: " << *addr;
        _exit(0);
    }
    server->start();

    return 0;
}

}

int main(int argc, char* argv[])
{
    gude::GudeServer* gd = new gude::GudeServer("./configs");
    if (gd->init(argc, argv))
        return gd->run();

    return 0;
}
