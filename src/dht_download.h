/**
 * brief: 抓取模块
 */
#ifndef __GUDE_DHT_DOWNLOAD_H
#define __GUDE_DHT_DOWNLOAD_H

#include "util.h"
#include "log.h"
#include "bencode.h"

#include "dht_config.h"

namespace gude
{

class DhtDownload
{
public:
    typedef std::shared_ptr<DhtDownload> ptr;

    DhtDownload(bifang::Address::ptr addr, const std::string& node_id,
        const std::string& v, const std::string& info_hash);

    std::string dump(const std::string& str);

    bool download();

private:
    bifang::Socket::ptr m_sock;
    bifang::Address::ptr m_addr;
    std::string m_node_id;
    std::string m_v;
    std::string m_info_hash;
};

}

#endif /*__GUDE_DHT_DOWNLOAD_H*/
