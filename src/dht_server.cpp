#include "dht_server.h"

namespace gude
{

SystemLogger();

DhtServer::DhtServer(bifang::IOManager* acceptWorker, bifang::IOManager* processWorker)
    :bifang::UdpServer(acceptWorker, processWorker)
{
}

// protected virtual
void DhtServer::handleClient(bifang::Socket::ptr sock)
{
    log_info << "handleClient: " << *sock;
    sock->setRecvTimeout(1000); // 设置超时时间
    DhtCrawler::ptr crawler(new DhtCrawler(sock, m_processWorker));
    crawler->ping_root();
    m_acceptWorker->addTimer(30 * 60 * 1000, std::bind(&DhtCrawler::ping_root, crawler));
    char* buf = new char[UDP_BUF_LEN];
    while (1)
    {
        struct sockaddr from;
        socklen_t fromlen = sizeof(from);
        int rc = sock->recvfrom(buf, UDP_BUF_LEN, &from, &fromlen, 0);
        if (rc > 0)
            crawler->periodic(buf, rc, from);
    }
    delete buf;
}

}
