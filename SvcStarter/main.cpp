#include <iostream>
#include "cSvcStarter.h"

int WINAPI WinMain( __in HINSTANCE hInstance, __in HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nCmdShow )
{
    LPWSTR* szArglist;
    int nArgs = 0;
    szArglist = CommandLineToArgvW( GetCommandLineW(), &nArgs );
    std::wstring a;
    for( int idx = 0; idx < nArgs; idx++ )
    {
        a += ( LPCWSTR )szArglist[ idx ];
        a += ( LPCWSTR )" ";
    }

    XString sArgList = a;

    cSvcMgr mgr;

    int nErr = mgr.serviceControl( nArgs, sArgList );

    if( nErr != 0 )
        return nErr;

    nErr = mgr.serviceStart();

    return nErr;
}