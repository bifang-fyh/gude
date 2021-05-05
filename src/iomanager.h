#ifndef __GUDE_IOMANAGER_H
#define __GUDE_IOMANAGER_H

#include "version.h"


#if defined(USE_EPOLL)
    #include "iomanager_epoll.h"
#else
    #error "please select IO Multiplexing model"
#endif


#endif /*__GUDE_IOMANAGER_H*/
