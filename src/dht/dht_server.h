/**
 * brief: DHT查询服务模块
 */
#ifndef __GUDE_DHT_SERVER_H
#define __GUDE_DHT_SERVER_H

#include "dht_search.h"
#include "dht_snatch.h"
#include "udpserver.h"


namespace gude
{

namespace dht
{

class DhtSearchServer : public UdpServer
{
public:
    typedef std::shared_ptr<DhtSearchServer> ptr;

    DhtSearchServer(IOManager* acceptWorker = IOManager::getThis(),
        IOManager* processWorker = IOManager::getThis());

    void ping_root(DhtSearch::ptr search);

protected:
    virtual void handleClient(Socket::ptr sock) override;
    
};

}

}

#endif /*__GUDE_DHT_SERVER_H*/
