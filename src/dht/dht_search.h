#ifndef __GUDE_DHT_H
#define __GUDE_DHT_H

#include <set>

#include "util.h"
#include "log.h"
#include "bencode.h"
#include "Assert.h"
#include "iomanager.h"
#include "dht_snatch.h"

namespace gude
{

namespace dht
{

#define MAX_SPEED    6000
#define AVG_SPEED    1500

class DhtSearch : public std::enable_shared_from_this<DhtSearch>
{
public:
    typedef std::shared_ptr<DhtSearch> ptr;

    enum MsgType
    {
        ERROR = 0,
        REPLY,
        PING,
        FIND_NODE,
        GET_PEERS,
        ANNOUNCE_PEER,
    };

    DhtSearch(Socket::ptr sock, IOManager* processWorker = IOManager::getThis());

private:
    bool control_speed();

    int dht_send(BEncode::Value& root, const struct sockaddr_in* sa);

    int send_ping(const struct sockaddr_in* sa, const std::string& id);
    int send_pong(const struct sockaddr_in* sa, const std::string& id, const std::string& tid);
    int send_find_node_r(const struct sockaddr_in* sa, const std::string& id, const std::string& tid);
    int send_get_peers_r(const struct sockaddr_in* sa, const std::string& id,
            const std::string& tid, const std::string& infohash);
    int send_announce_peer_r(const struct sockaddr_in* sa, const std::string& id, const std::string& tid);
    int send_error(const struct sockaddr_in* sa, const std::string& tid, int code, const std::string& message);

public:
    void start();
    void periodic(const char* buf, int len, struct sockaddr fromAddr);
    void ping_root();

private:
    void handle(std::string info_hash, struct sockaddr addr);

    int parse(const char* buf, int len, std::string& tid, std::string& id,
            std::string& info_hash, unsigned short& port, std::string& nodes);

protected:
    IOManager* m_processWorker;
    Socket::ptr m_sock;
    std::string m_id;
    std::string m_v;
    time_t m_now;
    time_t m_pre;
    time_t m_tickle;
};

}

}

#endif /*__GUDE_DHT_H*/
