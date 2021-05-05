/**
 * brief: 抓取模块
 */
#ifndef __GUDE_DHT_SNATCH_H
#define __GUDE_DHT_SNATCH_H

#include <ctype.h>

#include "util.h"
#include "log.h"
#include "bencode.h"

namespace gude
{

namespace dht
{

#define TORRENT_DIR    "torrent/"

class DhtSearchSnatch
{
public:
    typedef std::shared_ptr<DhtSearchSnatch> ptr;

    DhtSearchSnatch(Address::ptr addr, const std::string& node_id,
        const std::string& v, const std::string& info_hash);

    std::string dump(const std::string& str);

    bool download();

private:
    Socket::ptr m_sock;
    Address::ptr m_addr;
    std::string m_node_id;
    std::string m_v;
    std::string m_info_hash;
};

}

}

#endif /*__GUDE_DHT_SNATCH_H*/
