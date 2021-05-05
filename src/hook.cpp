#include "hook.h"
#include "log.h"
#include "iomanager.h"
#include "Assert.h"
#include "fdregister.h"
#include "config.h"


SystemLogger();

namespace gude
{

static Config<int64_t>::ptr g_tcp_connect_timeout =
           ConfigMgr::GetInstance()->get("system.tcp.connect_timeout", (int64_t)5000, "system tcp connect timeout config");

static thread_local bool t_hook_enable = false;

#define SYS_HOOK(XX) \
    XX(sleep) \
    XX(usleep) \
    XX(nanosleep) \
    XX(socket) \
    XX(connect) \
    XX(accept) \
    XX(read) \
    XX(readv) \
    XX(recv) \
    XX(recvfrom) \
    XX(recvmsg) \
    XX(write) \
    XX(writev) \
    XX(send) \
    XX(sendto) \
    XX(sendmsg) \
    XX(close) \
    XX(fcntl) \
    XX(setsockopt)

static void hook_init()
{
#define XX(name) g_sys_ ## name = (name ## _ptr)dlsym(RTLD_NEXT, #name);
    SYS_HOOK(XX);
#undef XX
}

static int64_t s_connect_timeout = -1;
struct HookInit
{
    HookInit()
    {
        hook_init();
        s_connect_timeout = g_tcp_connect_timeout->getValue();

        g_tcp_connect_timeout->addCallback(
            [](const int64_t& old_value, const int64_t& new_value)
            {
                //log_debug << "tcp connect timeout changed from "
                //    << old_value << " to " << new_value;
                s_connect_timeout = new_value;
            });
    }
};

static HookInit s_hook_init;

bool is_hook_enable()
{
    return t_hook_enable;
}

void hook_enable()
{
    t_hook_enable = true;
}

void hook_disable()
{
    t_hook_enable = false;
}

}

struct timer_info
{
    int cancelled = 0;
};

template<typename OriginFun, typename... Args>
static ssize_t do_io(int fd, OriginFun fun, const char* hook_fun_name,
                   uint32_t event, int timeout_so, Args&&... args)
{
    if (!gude::t_hook_enable)
        return fun(fd, std::forward<Args>(args)...);

    gude::FdData::ptr data = gude::FdMgr::GetInstance()->get(fd);
    if (!data)
    {
        return fun(fd, std::forward<Args>(args)...);
    }
    int64_t to = data->getTimeout(timeout_so);
    std::shared_ptr<timer_info> tinfo(new timer_info);

good:
    ssize_t n = fun(fd, std::forward<Args>(args)...);
    while (n == -1 && errno == EINTR)
        n = fun(fd, std::forward<Args>(args)...);

    if (n == -1 && errno == EAGAIN)
    {
        gude::IOManager* iom = gude::IOManager::getThis();
        gude::Timer::ptr timer;
        std::weak_ptr<timer_info> winfo(tinfo);

        if (to != -1) // 添加超时任务
        {
            timer = iom->addConditionTimer(to, [winfo, fd, iom, event]() {
                auto t = winfo.lock();
                if (!t || t->cancelled)
                    return;
                t->cancelled = ETIMEDOUT;
                iom->cancel(fd, (gude::IOContext::Event)(event));
            }, winfo);
        }

        bool ret = iom->add(fd, (gude::IOContext::Event)(event));
        if (LIKELY(ret))
        {
            gude::Fiber::yield();
            if (timer)
                timer->cancel();
            if (tinfo->cancelled)
            {
                errno = tinfo->cancelled;
                return -1;
            }
            goto good;
        }
        else
        {
            log_error << hook_fun_name << " add event("
                << fd << ", " << event << ")";
            if (timer)
                timer->cancel();
            return -1;
        }
    }

    return n;
}

extern "C"
{
#define XX(name) name ## _ptr g_sys_ ## name = nullptr;
    SYS_HOOK(XX);
#undef XX

unsigned int sleep(unsigned int seconds)
{
    if (UNLIKELY(!gude::t_hook_enable))
        return g_sys_sleep(seconds);

    gude::Fiber::ptr fiber = gude::Fiber::getThis();
    gude::IOManager* iom = gude::IOManager::getThis();
    iom->addTimer(seconds * 1000, std::bind((void(gude::Scheduler::*)
            (gude::Fiber::ptr))&gude::IOManager::schedule
            , iom, fiber));
    gude::Fiber::yield();
    return 0;
}

int usleep(useconds_t usec)
{
    if (UNLIKELY(!gude::t_hook_enable))
        return g_sys_usleep(usec);

    gude::Fiber::ptr fiber = gude::Fiber::getThis();
    gude::IOManager* iom = gude::IOManager::getThis();
    iom->addTimer(usec / 1000, std::bind((void(gude::Scheduler::*)
            (gude::Fiber::ptr))&gude::IOManager::schedule
            , iom, fiber));
    gude::Fiber::yield();
    return 0;
}

int nanosleep(const struct timespec *req, struct timespec *rem)
{
    if (UNLIKELY(!gude::t_hook_enable))
        return g_sys_nanosleep(req, rem);

    int timeout = req->tv_sec * 1000 + req->tv_nsec / 1000 /1000;
    gude::Fiber::ptr fiber = gude::Fiber::getThis();
    gude::IOManager* iom = gude::IOManager::getThis();
    iom->addTimer(timeout, std::bind((void(gude::Scheduler::*)
            (gude::Fiber::ptr))&gude::IOManager::schedule
            , iom, fiber));
    gude::Fiber::yield();
    return 0;
}

int socket(int domain, int type, int protocol)
{
    if (UNLIKELY(!gude::t_hook_enable))
        return g_sys_socket(domain, type, protocol);

    int fd = g_sys_socket(domain, type, protocol);
    if (fd < 0)
        return fd;
    gude::FdMgr::GetInstance()->add(fd);
    return fd;
}

int connect_with_timeout(int fd, const struct sockaddr* addr,
        socklen_t addrlen, int64_t timeout)
{
    if (UNLIKELY(!gude::t_hook_enable))
        return g_sys_connect(fd, addr, addrlen);

    gude::FdData::ptr data = gude::FdMgr::GetInstance()->get(fd);
    if (!data)
    {
        errno = EBADF;
        return -1;
    }

    int n = g_sys_connect(fd, addr, addrlen);
    if (n == 0 || n != -1 || errno != EINPROGRESS)
        return n;

    gude::IOManager* iom = gude::IOManager::getThis();
    gude::Timer::ptr timer;
    std::shared_ptr<timer_info> tinfo(new timer_info);
    std::weak_ptr<timer_info> winfo(tinfo);

    if (timeout != -1)
    {
        timer = iom->addConditionTimer(timeout, [winfo, fd, iom]() {
                auto t = winfo.lock();
                if (!t || t->cancelled)
                    return;
                t->cancelled = ETIMEDOUT;
                iom->cancel(fd, gude::IOContext::WRITE);
        }, winfo);
    }

    bool ret = iom->add(fd, gude::IOContext::WRITE);
    if (LIKELY(ret))
    {
        gude::Fiber::yield();
        if (timer)
            timer->cancel();
        if (tinfo->cancelled)
        {
            errno = tinfo->cancelled;
            return -1;
        }
    }
    else
    {
        log_error << "connect add event(" << fd << ", WRITE) error";
        if (timer)
            timer->cancel();
    }

    int error = 0;
    socklen_t len = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &len) == -1)
        return -1;

    return error ? -1 : 0;
}

int connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return connect_with_timeout(sockfd, addr, addrlen, gude::s_connect_timeout);
}

int accept(int s, struct sockaddr *addr, socklen_t *addrlen)
{
    int fd = do_io(s, g_sys_accept, "accept", gude::IOContext::READ,
        SO_RCVTIMEO, addr, addrlen);
    if (fd >= 0)
        gude::FdMgr::GetInstance()->add(fd);
    return fd;
}

ssize_t read(int fd, void *buf, size_t count)
{
    return do_io(fd, g_sys_read, "read", gude::IOContext::READ,
               SO_RCVTIMEO, buf, count);
}

ssize_t readv(int fd, const struct iovec *iov, int iovcnt)
{
    return do_io(fd, g_sys_readv, "readv", gude::IOContext::READ,
               SO_RCVTIMEO, iov, iovcnt);
}

ssize_t recv(int sockfd, void *buf, size_t len, int flags)
{
    return do_io(sockfd, g_sys_recv, "recv", gude::IOContext::READ,
               SO_RCVTIMEO, buf, len, flags);
}

ssize_t recvfrom(int sockfd, void *buf, size_t len, int flags,
            struct sockaddr *src_addr, socklen_t *addrlen)
{
    return do_io(sockfd, g_sys_recvfrom, "recvfrom", gude::IOContext::READ,
               SO_RCVTIMEO, buf, len, flags, src_addr, addrlen);
}

ssize_t recvmsg(int sockfd, struct msghdr *msg, int flags)
{
    return do_io(sockfd, g_sys_recvmsg, "recvmsg", gude::IOContext::READ,
               SO_RCVTIMEO, msg, flags);
}

ssize_t write(int fd, const void *buf, size_t count)
{
    return do_io(fd, g_sys_write, "write", gude::IOContext::WRITE,
               SO_SNDTIMEO, buf, count);
}

ssize_t writev(int fd, const struct iovec *iov, int iovcnt)
{
    return do_io(fd, g_sys_writev, "writev", gude::IOContext::WRITE,
               SO_SNDTIMEO, iov, iovcnt);
}

ssize_t send(int s, const void *msg, size_t len, int flags)
{
    return do_io(s, g_sys_send, "send", gude::IOContext::WRITE,
               SO_SNDTIMEO, msg, len, flags);
}

ssize_t sendto(int s, const void *msg, size_t len, int flags,
    const struct sockaddr *to, socklen_t tolen)
{
    return do_io(s, g_sys_sendto, "sendto", gude::IOContext::WRITE,
               SO_SNDTIMEO, msg, len, flags, to, tolen);
}

ssize_t sendmsg(int s, const struct msghdr *msg, int flags)
{
    return do_io(s, g_sys_sendmsg, "sendmsg", gude::IOContext::WRITE,
               SO_SNDTIMEO, msg, flags);
}

int close(int fd)
{
    if (UNLIKELY(!gude::t_hook_enable))
        return g_sys_close(fd);

    gude::FdData::ptr data = gude::FdMgr::GetInstance()->get(fd);
    if (data)
    {
        auto iom = gude::IOManager::getThis();
        if (iom)
            iom->cancel(fd);
        gude::FdMgr::GetInstance()->del(fd);
    }

    return g_sys_close(fd);
}

int fcntl(int fd, int cmd, ...)
{
    va_list va;
    va_start(va, cmd);
    int ret = -1;

    switch (cmd)
    {
        case F_SETFL:
        {
            int arg = va_arg(va, int);
            va_end(va);
            gude::FdData::ptr data = gude::FdMgr::GetInstance()->get(fd);
            if (data && gude::t_hook_enable)
                arg |= O_NONBLOCK;
            ret = g_sys_fcntl(fd, cmd, arg);
            break;
        }

        case F_GETFL:
        case F_GETFD:
        case F_GETOWN:
        case F_GETSIG:
        case F_GETLEASE:
#ifdef F_GETPIPE_SZ
        case F_GETPIPE_SZ:
#endif
        {
            ret = g_sys_fcntl(fd, cmd);
            break;
        }

        case F_DUPFD:
        case F_DUPFD_CLOEXEC:
        case F_SETFD:
        case F_SETOWN:
        case F_SETSIG:
        case F_SETLEASE:
        case F_NOTIFY:
#ifdef F_SETPIPE_SZ
        case F_SETPIPE_SZ:
#endif
        {
            int arg = va_arg(va, int);
            va_end(va);
            ret = g_sys_fcntl(fd, cmd, arg); 
            break;
        }

        case F_SETLK:
        case F_SETLKW:
        case F_GETLK:
        {
            struct flock* arg = va_arg(va, struct flock*);
            ret = g_sys_fcntl(fd, cmd, arg);
            break;
        }

        case F_GETOWN_EX:
        case F_SETOWN_EX:
        {
            struct f_owner_exlock* arg = va_arg(va, struct f_owner_exlock*);
            ret = g_sys_fcntl(fd, cmd, arg);
            break;
        }

        default:
        {
            ret = g_sys_fcntl(fd, cmd);
            break;
        }
    }

    va_end(va);
    return ret;
}

int setsockopt(int sockfd, int level, int optname, const void *optval,
        socklen_t optlen)
{
    if (UNLIKELY(!gude::t_hook_enable))
        return g_sys_setsockopt(sockfd, level, optname, optval, optlen);

    if (level == SOL_SOCKET)
    {
        if (optname == SO_RCVTIMEO || optname == SO_SNDTIMEO)
        {
            gude::FdData::ptr data = gude::FdMgr::GetInstance()->get(sockfd);
            if (data)
            {
                const timeval* v = (const timeval*)optval;
                data->setTimeout(optname, v->tv_sec * 1000 + v->tv_usec / 1000);
            }
        }
    }

    return g_sys_setsockopt(sockfd, level, optname, optval, optlen);
}

}
