#pragma once

#include <map>
#include <mutex>
#include <thread>

#include "String/SharedXString.h"

typedef struct _tyService
{
    XString        sUniqueName;
    XString        sFileFullPath;
    bool           isDefence = false;
    bool           isUpdate = false;
    bool           isStop = false;
    bool           bRunSystemProcess = false;
} SERVICE_ITEM;

class cSvcMgr 
{
public:
    cSvcMgr();
    ~cSvcMgr();

    void ServiceWorker( const XString& sName );

    void MainWorker();

private:
    std::map< XString, SERVICE_ITEM > _mapServiceItem;
    std::map< XString, std::thread >  _mapServiceItemThread;
    bool                              _isStop = false;
    std::mutex                        _lck;
};
