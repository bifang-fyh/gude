#include "dht_snatch.h"

namespace gude
{

namespace dht
{

SystemLogger();

static std::atomic<size_t> g_count = {0};

DhtSearchSnatch::DhtSearchSnatch(Address::ptr addr, const std::string& node_id,
                    const std::string& v, const std::string& info_hash)
    :m_addr(addr)
    ,m_node_id(node_id)
    ,m_v(v)
    ,m_info_hash(info_hash)
{
    m_sock = Socket::createTCP(addr);
}

std::string DhtSearchSnatch::dump(const std::string& str)
{
    std::stringstream ss;
    size_t i = 0;
    while (i < str.size())
    {
        if (i != 0)
            ss << std::endl;
        ss << std::hex;
        for (size_t j = i; j < str.size() && j < i + 16; j++)
            ss << std::setw(2) << std::setfill('0') << (uint32_t)(uint8_t)str[j] << " ";
        ss << std::endl;
        ss << std::dec;
        for (size_t j = i; j < str.size() && j < i + 16; j++)
        {
            int c = (uint8_t)str[j];
            if (isprint(c))
                ss << str[j] << "  ";
            else
                ss << ".  ";
        }
        i += 16;
    }
    return ss.str();
}

#if 0
void DhtSearchSnatch::escape(std::string& str)
{
    size_t i = 0;
    while (i < str.size())
    {
        size_t pos = str.find("'", i);
        if (pos == std::string::npos)
            break;
        str.insert(pos, 1, '\'');
        i = pos + 2;
    }
}
#endif

bool DhtSearchSnatch::download()
{
#define BUF_LEN       (20 * 1024)
#define PIECE_SIZE    (16 * 1024)
#define COMMON_PART   "hash(" << hash << "), address[" << *m_addr << "], "

    std::string hash = StringUtil::toHexString(m_info_hash);

    char* buf = new char[BUF_LEN];
    if (!m_sock->connect(m_addr))
    {
        delete buf;
        return false;
    }
    m_sock->setSendTimeout(19 * 1000);
    m_sock->setRecvTimeout(31 * 1000);

    // 握手
    std::string handshake_message;
    handshake_message.resize(28);
    handshake_message[0] = 19;
    memcpy(&handshake_message[1], "BitTorrent protocol", 19);
    char ext[8];
    memset(ext, 0x00, sizeof(ext));
    ext[5] = 0x10;
    ext[7] = 0x04;
    memcpy(&handshake_message[20], ext, 8);
    handshake_message += m_info_hash + m_node_id;
    m_sock->send(&handshake_message[0], handshake_message.size());
    int len = m_sock->recv(buf, BUF_LEN);
    if (len < 68)
    {
        log_debug << COMMON_PART << "(handshake) message size=" << len
            << " is too short(must be >= 68)";
        delete buf;
        return false;
    }
    std::string handshake_reply(buf, 68);
    std::string ext_message;
    if (len > 68)
        ext_message = std::string(buf + 68, len - 68);
    if (handshake_reply.substr(0, 20) != handshake_message.substr(0, 20))
    {
        log_debug << COMMON_PART << "(handshake) protocol fail, message:"
            << std::endl << dump(handshake_reply);
        delete buf;
        return false;
    }
    if ((int)handshake_reply[25] & 0x10 == 0)
    {
        log_debug << COMMON_PART << "(handshake) peer does not support extension protocol, message:"
            << std::endl << dump(handshake_reply);
        delete buf;
        return false;
    }
    if ((int)handshake_reply[27] & 0x04 == 0)
    {
        log_debug << COMMON_PART << "(handshake) peer does not support fast protocol, message:"
            << std::endl << dump(handshake_reply);
        delete buf;
        return false;
    }
    // 扩展握手
    std::string ext_handshake_message;
    ext_handshake_message.append(1, 20);
    ext_handshake_message.append(1, 0);
    ext_handshake_message += "d1:md11:ut_metadatai2ee1:v" + std::to_string(m_v.size()) + ":" + m_v + "e";
    std::string ext_handshake_message_size_str;
    ext_handshake_message_size_str.resize(4);
    uint32_t ext_handshake_message_size = ext_handshake_message.size();
    ext_handshake_message_size = littleByteSwap(ext_handshake_message_size);
    memcpy(&ext_handshake_message_size_str[0], &ext_handshake_message_size, 4);
    ext_handshake_message = ext_handshake_message_size_str + ext_handshake_message;
    m_sock->send(&ext_handshake_message[0], ext_handshake_message.size());
    len = 0;
    while (1)
    {
        int cur_len = m_sock->recv(buf + len, BUF_LEN - len);
        if (cur_len <= 0)
            break;
        len += cur_len;
        if (len >= BUF_LEN)
            break;
    }
    std::string ext_reply;
    if (len > 0)
        ext_reply = ext_message + std::string(buf, len);
    else if (!ext_message.empty())
        ext_reply = ext_message;
    else
    {
        log_debug << COMMON_PART << "(ext handshake) fail";
        delete buf;
        return false;
    }
    // 摘取数据
    // ut_metadata
    size_t pos = ext_reply.find("ut_metadata");
    if (pos == std::string::npos)
    {
        log_debug << COMMON_PART << "(ext handshake) parse ut_metadata fail, message:"
            << std::endl << dump(ext_reply);
        delete buf;
        return false;
    }
    pos += 12;
    size_t pos_e = ext_reply.find("e", pos);
    if (pos_e == std::string::npos)
    {
        log_debug << COMMON_PART << "(ext handshake) parse ut_metadata fail, message:"
            << std::endl << dump(ext_reply);
        delete buf;
        return false;
    }
    std::string ut_metadata_str = ext_reply.substr(pos, pos_e - pos);
    uint32_t ut_metadata = atoi(ut_metadata_str.c_str());

    // metadata_size
    pos = ext_reply.find("metadata_size");
    if (pos == std::string::npos)
    {
        log_debug << COMMON_PART << "(ext handshake) parse metadata_size fail, message:"
            << std::endl << dump(ext_reply);
        delete buf;
        return false;
    }
    pos += 14;
    pos_e = ext_reply.find("e", pos);
    if (pos_e == std::string::npos)
    {
        log_debug << COMMON_PART << "(ext handshake) parse metadata_size fail, message:"
            << std::endl << dump(ext_reply);
        delete buf;
        return false;
    }
    std::string metadata_size_str = ext_reply.substr(pos, pos_e - pos);
    int64_t metadata_size = atoll(metadata_size_str.c_str());

    //log_info << "ut_metadata:" << ut_metadata << ", metadata_size:" << metadata_size;

    // get metadata
    std::string data;
    int piece = 0;
    while (metadata_size > 0)
    {
        std::string get_metadata_message;
        get_metadata_message.append(1, 20);
        get_metadata_message.append(1, 2);
        get_metadata_message += "d8:msg_typei0e5:piecei" + std::to_string(piece) + "ee";
        std::string get_metadata_message_size_str;
        get_metadata_message_size_str.resize(4);
        uint32_t get_metadata_message_size = get_metadata_message.size();
        get_metadata_message_size = littleByteSwap(get_metadata_message_size);
        memcpy(&get_metadata_message_size_str[0], &get_metadata_message_size, 4);
        get_metadata_message = get_metadata_message_size_str + get_metadata_message;
        m_sock->send(&get_metadata_message[0], get_metadata_message.size());
        len = 0;
        while (1)
        {
            int cur_len = m_sock->recv(buf + len, BUF_LEN - len);
            if (cur_len <= 0)
                break;
            len += cur_len;
            if (len >= BUF_LEN)
                break;
        }
        if (len <= 0)
            break;

        int i = 6;
        while (i < len - 1)
        {
            if (buf[i] == 'e' && buf[i + 1] == 'e')
            {
                i += 2;
                break;
            }
            i++;
        }

        if (i < len)
        {
            data.append(buf + i, len - i);
            metadata_size -= (len - i);
            piece++;
        }
        else
        {
            log_debug << COMMON_PART << "get metadata message is invaild, message:"
                << std::endl << dump(std::string(buf, len));
            if (data.empty())
            {
                delete buf;
                return false;
            }
            else
            {
                data.append(buf, len);
                break;
            }
        }
    }
    delete buf;

    if (data.size() < 50)
    {
        log_error << COMMON_PART << "get metadata size=" << data.size() << " is too short";
        return false;
    }

    std::string torrent_name = TORRENT_DIR + hash + ".torrent";
    if (FileUtil::__lstat(torrent_name))
    {
        log_info << "torrent file(" << torrent_name << ") is exists";
        return false;
    }
    std::ofstream ofs(torrent_name);
    if (ofs)
        ofs.write(&data[0], data.size());
    else
    {
        log_error << COMMON_PART << "open file fail";
        return false;
    }
    log_info << COMMON_PART << "get metadata successful, count:" << ++g_count;
    return true;
}

}

}
