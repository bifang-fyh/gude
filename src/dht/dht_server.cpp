#include "dht_server.h"

namespace gude
{

namespace dht
{

SystemLogger();

DhtSearchServer::DhtSearchServer(IOManager* acceptWorker, IOManager* processWorker)
    :UdpServer(acceptWorker, processWorker)
{
}

void DhtSearchServer::ping_root(DhtSearch::ptr search)
{
    log_debug << "ping root";
    search->ping_root();
}

// protected virtual
void DhtSearchServer::handleClient(Socket::ptr sock)
{
#define BUF_LEN 8192

    log_info << "handleClient: " << *sock;
    sock->setRecvTimeout(1000); // 设置超时时间
    DhtSearch::ptr search(new DhtSearch(sock, m_processWorker));
    search->ping_root();
    m_acceptWorker->addTimer(30 * 60 * 1000, std::bind(&DhtSearchServer::ping_root, this, search));
    char* buf = new char[BUF_LEN];
    while (1)
    {
        struct sockaddr from;
        socklen_t fromlen = sizeof(from);
        int rc = sock->recvfrom(buf, BUF_LEN, &from, &fromlen, 0);
        if (rc > 0)
            search->periodic(buf, rc, from);
    }
    delete buf;

#undef BUF_LEN
}

}

}
