#ifndef __GUDE_NONCOPYABLE_H
#define __GUDE_NONCOPYABLE_H

namespace gude
{

struct Noncopyable
{
    Noncopyable() = default;

    ~Noncopyable() = default;

    Noncopyable(const Noncopyable&) = delete;

    Noncopyable& operator=(const Noncopyable&) = delete;
};

}

#endif /*__GUDE_NONCOPYABLE_H*/

