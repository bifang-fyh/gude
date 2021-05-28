/**
 * brief: DHT查询服务模块
 */
#ifndef __DHT_SERVER_H
#define __DHT_SERVER_H

#include "dht_config.h"
#include "dht_crawler.h"
#include "udpserver.h"


namespace gude
{

class DhtServer : public bifang::UdpServer
{
public:
    typedef std::shared_ptr<DhtServer> ptr;

    DhtServer(bifang::IOManager* acceptWorker = bifang::IOManager::getThis(),
        bifang::IOManager* processWorker = bifang::IOManager::getThis());

protected:
    virtual void handleClient(bifang::Socket::ptr sock) override;
    
};

}

#endif /*__DHT_SERVER_H*/
