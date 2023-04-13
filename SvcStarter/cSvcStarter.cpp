#include "cSvcStarter.h"

#include <codecvt>
#include <fstream>
#include <iostream>

#include "String/SharedXString.h"
#include "Utils/SharedFile.h"
#include "Windows/SharedWinTools.h"

cSvcMgr::cSvcMgr()
{
}

cSvcMgr::~cSvcMgr()
{}

int cSvcMgr::serviceControl( int argc, XString argv )
{
    int nErr = 0;
    auto vecArgv = argv.split( " " );
    do
    {
        if( argc > 1 )
        {
            std::wstring sType = vecArgv[ 1 ];
            // Install
            if( sType.compare( L"-i" ) == 0 )
            {
                if( argc < 4 )
                    break;

                if( Shared::File::IsExistFile( vecArgv[ 3 ] ) == false )
                    break;

                bool isSuccess = Shared::Windows::CreateWindowsService( vecArgv[ 2 ], vecArgv[ 2 ], vecArgv[ 3 ] );

                if( isSuccess == false )
                    nErr = ::GetLastError();
                else
                    _name = vecArgv[ 2 ];

                nErr = serviceStart();
            }
            else if( sType.compare( L"-h" ) == 0 )
            {
                std::cout << "===============================================" << std::endl;
                std::cout << "INSTALL SERVICE => -i [SVCNAME] [FILEPATH]" << std::endl;
                std::cout << "START   SERVICE => -s [SVCNAME] [FILEPATH]" << std::endl;
                std::cout << "DELETE  SERVICE => -d [SVCNAME] [FILEPATH]" << std::endl;
                std::cout << "===============================================" << std::endl;
            }
            else if( sType.compare( L"-r" ) == 0 )
            {
                if( argc < 3 )
                    break;

                if( Shared::Windows::IsExistService( vecArgv[ 2 ] ) == false )
                    break;

                if( Shared::Windows::IsRunningService( vecArgv[ 2 ] ) == true )
                    break;

                _name = vecArgv[ 2 ];

                nErr = serviceStart();
            }
            else if( sType.compare( L"-d" ) == 0 )
            {
                if( argc < 3 )
                    break;

                if( Shared::Windows::IsExistService( vecArgv[ 2 ] ) == false )
                    break;

                _name = vecArgv[ 2 ];

                nErr = serviceDelete();
            }
        }
    }
    while( false );

    return nErr;
}

int cSvcMgr::serviceStart()
{
    bool isSuccess = Shared::Windows::RunWindowsService( _name );

    if( isSuccess == false )
        return ::GetLastError();
    else
        return 0;
}

int cSvcMgr::serviceDelete()
{
    bool isSuccess = Shared::Windows::RemoveWindowsService( _name );

    if( isSuccess == false )
        return ::GetLastError();
    else
        return 0;
}
