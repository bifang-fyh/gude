#include "dht_search.h"

namespace gude
{

namespace dht
{

SystemLogger();

#define MIN(x, y)    ((x) <= (y) ? (x) : (y))

DhtSearch::DhtSearch(Socket::ptr sock, IOManager* processWorker)
    :m_sock(sock)
    ,m_processWorker(processWorker)
{
    static int rr = Srand();
    std::string r = StringUtil::randomString(27);
    m_id = CryptUtil::sha1sum(r);
    m_v = "FTDS";
    m_now = time(0);
    m_pre = m_now;
    m_tickle = MAX_SPEED;
}

// private 
bool DhtSearch::control_speed()
{
    if (m_tickle == 0)
    {
        m_tickle = MIN(MAX_SPEED, AVG_SPEED * (m_now - m_pre));
        m_pre = m_now;
    }

    if (m_tickle == 0)
        return false;

    m_tickle--;
    return true;
}

// private
int DhtSearch::dht_send(BEncode::Value& root, const struct sockaddr_in* sa)
{
    std::string str;
    if (!BEncode::encode(str, &root))
    {
        log_error << "bencode fail";
        return -1;
    }
    Address::ptr remote(new IPv4Address(*sa));
    log_debug << "send to: " << remote->toString();
    return m_sock->sendto(&str[0], str.size(), remote);
}

// private
int DhtSearch::send_ping(const struct sockaddr_in* sa, const std::string& id)
{
    BEncode::Value root(BEncode::Value::BCODE_DICTIONARY);
    BEncode::Value value(BEncode::Value::BCODE_DICTIONARY);
    std::string v_id;
    if (!id.empty())
        v_id = id.substr(0, 18) + m_id.substr(18, 2);
    else
        v_id = m_id;
    value["id"] = BEncode::Value(v_id);
    root["a"] = value;
    root["q"] = BEncode::Value("ping");
    root["t"] = BEncode::Value("pn");
    root["v"] = BEncode::Value(m_v);
    root["y"] = BEncode::Value("q");
    return dht_send(root, sa);
}

// private
int DhtSearch::send_pong(const struct sockaddr_in* sa, const std::string& id,
                   const std::string& tid)
{
    BEncode::Value root(BEncode::Value::BCODE_DICTIONARY);
    BEncode::Value value(BEncode::Value::BCODE_DICTIONARY);
    std::string v_id;
    if (!id.empty())
        v_id = id.substr(0, 18) + m_id.substr(18, 2);
    else
        v_id = m_id;
    value["id"] = BEncode::Value(v_id);
    root["r"] = value;
    root["t"] = BEncode::Value(tid);
    root["v"] = BEncode::Value(m_v);
    root["y"] = BEncode::Value("r");
    return dht_send(root, sa);
}

// private
int DhtSearch::send_find_node_r(const struct sockaddr_in* sa,
                   const std::string& id, const std::string& tid)
{
    BEncode::Value root(BEncode::Value::BCODE_DICTIONARY);
    BEncode::Value value(BEncode::Value::BCODE_DICTIONARY);
    std::string v_id;
    if (!id.empty())
        v_id = id.substr(0, 18) + m_id.substr(18, 2);
    else
        v_id = m_id;
    value["id"] = BEncode::Value(v_id);
    std::string nodes;
    value["nodes"] = BEncode::Value(nodes);
    root["r"] = value;
    root["t"] = BEncode::Value(tid);
    root["v"] = BEncode::Value(m_v);
    root["y"] = BEncode::Value("r");
    return dht_send(root, sa);
}

// private
int DhtSearch::send_get_peers_r(const struct sockaddr_in* sa, const std::string& id,
                   const std::string& tid, const std::string& infohash)
{
    BEncode::Value root(BEncode::Value::BCODE_DICTIONARY);
    BEncode::Value value(BEncode::Value::BCODE_DICTIONARY);
    std::string v_id;
    if (!id.empty())
        v_id = id.substr(0, 18) + m_id.substr(18, 2);
    else
        v_id = m_id;
    value["id"] = BEncode::Value(v_id);
    std::string nodes;
    value["nodes"] = BEncode::Value(nodes);
    value["token"] = infohash.substr(0, 2);
    root["r"] = value;
    root["t"] = BEncode::Value(tid);
    root["v"] = BEncode::Value(m_v);
    root["y"] = BEncode::Value("r");
    return dht_send(root, sa);
}

// private
int DhtSearch::send_announce_peer_r(const struct sockaddr_in* sa,
                   const std::string& id, const std::string& tid)
{
    BEncode::Value root(BEncode::Value::BCODE_DICTIONARY);
    BEncode::Value value(BEncode::Value::BCODE_DICTIONARY);
    std::string v_id;
    if (!id.empty())
        v_id = id.substr(0, 18) + m_id.substr(18, 2);
    else
        v_id = m_id;
    value["id"] = BEncode::Value(v_id);
    root["r"] = value;
    root["t"] = BEncode::Value(tid);
    root["v"] = BEncode::Value(m_v);
    root["y"] = BEncode::Value("r");
    return dht_send(root, sa);
}

// private
int DhtSearch::send_error(const struct sockaddr_in* sa, const std::string& tid,
                   int code, const std::string& message)
{
    BEncode::Value root(BEncode::Value::BCODE_DICTIONARY);
    BEncode::Value value(BEncode::Value::BCODE_LIST);
    value.append(BEncode::Value(code));
    value.append(BEncode::Value(message));
    root["e"] = value;
    root["t"] = tid;
    root["v"] = BEncode::Value(m_v);
    root["y"] = BEncode::Value("e");
    return dht_send(root, sa);
}

void DhtSearch::periodic(const char* buf, int len, struct sockaddr fromAddr)
{
    m_now = time(0);
    std::string tid, id, info_hash, nodes;
    unsigned short port;
    int message = parse(buf, len, tid, id, info_hash, port, nodes);
    if (message < 0 || id.empty()) 
    {
        log_debug << "unparseparseable message";
        return;
    }
    if (id == m_id)
    {
        log_debug << "received message from self";
        return;
    }
    if (message > REPLY && !control_speed())
        return;

    switch (message) 
    {
        case REPLY:
            if (!nodes.empty()) 
            {
                for (size_t i = 0; i < nodes.size() / 26; i++) 
                {
                    std::string ni = nodes.substr(i * 26, 26);
                    std::string tmp_id = ni.substr(0, 20);
                    if (tmp_id == m_id)
                        continue;
                    struct sockaddr_in sin;
                    memset(&sin, 0x00, sizeof(sin));
                    sin.sin_family = AF_INET;
                    memcpy(&sin.sin_addr, &ni[20], 4);
                    memcpy(&sin.sin_port, &ni[24], 2);
                    send_ping(&sin, tmp_id);
                }
            }
            break;

        case PING:
            send_pong((struct sockaddr_in*)&fromAddr, id, tid);
            break;

        case FIND_NODE:
            send_find_node_r((struct sockaddr_in*)&fromAddr, id, tid);
            break;

        case GET_PEERS:
            if (info_hash.empty())
            {
                log_error << "get_peers with no info_hash";
                send_error((struct sockaddr_in*)&fromAddr, tid, 203, "Get_peers with no info_hash");
            }
            else
            {
                send_get_peers_r((struct sockaddr_in*)&fromAddr, id, tid, info_hash);
                std::string torrent_name = TORRENT_DIR + StringUtil::toHexString(info_hash);
                if (FileUtil::__lstat(torrent_name))
                {
                    //log_info << "torrent file(" << torrent_name << ") is exists";
                    return;
                }
                m_processWorker->schedule(std::bind(&DhtSearch::handle, shared_from_this(), info_hash, fromAddr));
            }
            break;

        case ANNOUNCE_PEER:
            if (info_hash.empty())
            {
                log_error << "announce_peer with no info_hash";
                send_error((struct sockaddr_in*)&fromAddr, tid, 203, "Announce_peer with no info_hash");
            }
            else if (port == 0)
            {
                log_error << "announce_peer with forbidden port 0";
                send_error((struct sockaddr_in*)&fromAddr, tid, 203, "Announce_peer with forbidden port number");
            }
            else
            {
                send_announce_peer_r((struct sockaddr_in*)&fromAddr, id, tid);
                std::string torrent_name = TORRENT_DIR + StringUtil::toHexString(info_hash);
                if (FileUtil::__lstat(torrent_name))
                {
                    //log_info << "torrent file(" << torrent_name << ") is exists";
                    return;
                }
                m_processWorker->schedule(std::bind(&DhtSearch::handle, shared_from_this(), info_hash, fromAddr));
            }
            break;
    }
}

void DhtSearch::ping_root()
{
    std::vector<std::pair<const char*, const char*>> ip_addr = 
    {
        {"router.utorrent.com",    "6881"},
        {"router.bittorrent.com",  "6881"},
        {"dht.transmissionbt.com", "6881"}
    };

    for (auto addr : ip_addr)
    {
        struct addrinfo hints, *info;
        memset(&hints, 0, sizeof(hints));
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_family = AF_UNSPEC;

        int error = getaddrinfo(addr.first, addr.second, &hints, &info);
        if (error)
        {
            log_error << "getaddrinfo fail, error=" << error << ", errstr=" << gai_strerror(error);
        }
        else
        {
            struct addrinfo* p = info;
            while (p)
            {
                if (p->ai_family == AF_INET)
                {
                    send_ping((struct sockaddr_in*)p->ai_addr, "");
                    log_debug << addr.first << ":" << addr.second << " is AF_INET";
                }
                else
                {
                    log_debug << addr.first << ":" << addr.second << " is no support the family(" << p->ai_family << ")";
                }

                p = p->ai_next;
            }
            freeaddrinfo(info);
        }
    }
}

// private
void DhtSearch::handle(std::string info_hash, struct sockaddr addr)
{
    struct sockaddr_in sin;
    memcpy(&sin, &addr, sizeof(struct sockaddr));
    IPv4Address::ptr ipv4_addr(new IPv4Address(sin));
    DhtSearchSnatch::ptr snatch(new DhtSearchSnatch(ipv4_addr, m_id, m_v, info_hash));
    snatch->download();
}

// private
int DhtSearch::parse(const char* buf, int len, std::string& tid, std::string& id,
                   std::string& info_hash, unsigned short& port, std::string& nodes)
{
#define XX(str) \
    log_error << str; \
    return -1

    int ret;
    BEncode::Value root;
    size_t start = 0;
    if (BEncode::decode(buf, start, len, &root) || root.getType() != BEncode::Value::BCODE_DICTIONARY)
    {
        XX("bencode message is invalid");
    }

    // tid(始终在顶层)
    {
        auto value = root.find("t");
        if (value != root.end())
        {
            if (value->getType() != BEncode::Value::BCODE_STRING)
            {
                XX("\"t\" value is must be string");
            }
            tid = value->asString();
        }
    }

    // y(始终在顶层)
    auto type_y = root.find("y");
    if (type_y != root.end() && type_y->getType() == BEncode::Value::BCODE_STRING)
    {
        std::string value = type_y->asString();
        if (value == "r")
            ret = REPLY;
        else if (value == "e")
        {
            XX("remote reply ERROR value");
        }
        else if (value == "q")
        {
            auto type_q = root.find("q");
            if (type_q != root.end() && type_q->getType() == BEncode::Value::BCODE_STRING)
            {
                std::string v = type_q->asString();
                if (v == "ping")
                    ret = PING;
                else if (v == "find_node")
                    ret = FIND_NODE;
                else if (v == "get_peers")
                    ret = GET_PEERS;
                else if (v == "announce_peer")
                    ret = ANNOUNCE_PEER;
                else if (v == "vote" || v == "sample_infohashes")
                    return -1;
                else
                {
                    XX("\"q\" value(" + v + ") is invaild");
                }
            }
            else
            {
                XX("not found \"q\" value");
            }
        }
        else
        {
            XX("\"y\" value(" + value + ") is invaild");
        }
    }
    else
    {
        XX("not found \"y\" value");
    }

    BEncode::Value::iterator body_value;
    if (ret == REPLY)
    {
        body_value = root.find("r");
        if (body_value == root.end() || body_value->getType() != BEncode::Value::BCODE_DICTIONARY)
        {
            XX("not found \"r\" value");
        }
    }
    else
    {
        body_value = root.find("a");
        if (body_value == root.end() || body_value->getType() != BEncode::Value::BCODE_DICTIONARY)
        {
            XX("not found \"a\" value");
        }
    }

    // id
    {
        auto value = body_value->find("id");
        if (value != body_value->end())
        {
            if (value->getType() != BEncode::Value::BCODE_STRING)
            {
                XX("\"id\" value is must be string");
            }
            id = value->asString();
            if (id.size() != 20)
                id.clear();
        }
        else
            id.clear();
    }

    // info_hash
    {
        auto value = body_value->find("info_hash");
        if (value != body_value->end())
        {
            if (value->getType() != BEncode::Value::BCODE_STRING)
            {
                XX("\"info_hash\" value is must be string");
            }
            info_hash = value->asString();
            if (info_hash.size() != 20)
                info_hash.clear();
        }
        else
            info_hash.clear();
    }

    // port
    {
        auto value = body_value->find("port");
        if (value != body_value->end())
        {
            if (value->getType() != BEncode::Value::BCODE_INTEGER)
            {
                XX("\"port\" value is must be int");
            }
            port = (unsigned short)(value->asInt());
        }
        else
            port = 0;
    }

    // nodes
    {
        auto value = body_value->find("nodes");
        if (value != body_value->end())
        {
            if (value->getType() != BEncode::Value::BCODE_STRING)
            {
                XX("\"nodes\" value is must be string");
            }
            nodes = value->asString();
        }
        else
            nodes.clear();
    }
    return ret;

#undef XX
}

}

}
