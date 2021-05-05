/**
 * brief: 单例类封装
 */
#ifndef __GUDE_SINGLETON_H
#define __GUDE_SINGLETON_H

namespace gude
{

template<typename T>
struct Singleton
{
    static T* GetInstance()
    {
        static T m;
        return &m;
    }
};

}

#endif /*__GUDE_SINGLETON_H*/
