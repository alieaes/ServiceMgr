#pragma once

#include "String/SharedXString.h"

class cSvcMgr
{
public:
    cSvcMgr();
    ~cSvcMgr();

    int serviceControl( int argc, XString argv );
    int serviceStart();
    int serviceDelete();

private:
    XString _name;
};
