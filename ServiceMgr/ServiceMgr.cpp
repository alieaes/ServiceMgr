#include "stdafx.h"
#include "ServiceMgr.hpp"

#include <codecvt>
#include <fstream>

#include "Json/nlohmann/json.hpp"
#include "String/SharedXString.h"
#include "Utils/SharedFile.h"
#include "Utils/SharedProcess.h"

cSvcMgr::cSvcMgr()
{
    MainWorker();
}

cSvcMgr::~cSvcMgr()
{
    _isStop = true;
}

void cSvcMgr::ServiceWorker( const XString& sName )
{
    while( _isStop == false )
    {
        SERVICE_ITEM item;

        {
            std::lock_guard<std::mutex> lck( _lck );
            item = _mapServiceItem[ sName ];
        }

        if( item.isUpdate == true )
        {
            for( int idx = 0; idx < 1000; idx++ )
            {
                if( item.isStop == true )
                    break;

                Sleep( 10 );
            }
        }

        if( item.isStop == true )
            break;

        if( Shared::File::IsExistFile( item.sFileFullPath ) == false )
        {
            Sleep( 1000 );
            continue;
        }

        auto sFileName = Shared::File::FindFileName( item.sFileFullPath );

        if( Shared::Process::GetPIDFromProcessName( sFileName ) == 0 )
        {
            if( item.bRunSystemProcess == true )
                Shared::Process::ExecutionUsingWinLogon( item.sFileFullPath );
            else
                Shared::Process::ExecutionAsAdministrator( item.sFileFullPath );
        }

        for( int idx = 0; idx < 1000; idx++ )
        {
            if( item.isStop == true )
                break;

            Sleep( 10 );
        }

        if( item.isStop == true )
            break;
    }
}

void cSvcMgr::MainWorker()
{
    XString sJsonFile = Shared::File::GetCurrentPath( true ) + "ServiceList.json";
    bool bStart = true;

    while( _isStop == false )
    {
        if( bStart == false )
            Sleep( 1000 * 10 );
        else
            bStart = false;

        nlohmann::json json;
        std::ifstream i;

        i.open( sJsonFile.toWString() );

        if( i.fail() == true )
        {
            Sleep( 1000 );
            continue;
        }
        else
        {
            i >> json;
            i.close();
        }

        bool isExist = false;

        isExist = json.contains( "SVC_LIST" );

        if( isExist == false ) 
            continue;

        auto svcList = json[ "SVC_LIST" ];

        std::vector<XString> vecUniqueName;

        for( int idx = 0; idx < svcList.size(); idx++ )
        {
            isExist &= svcList[ idx ].contains( "uniqueName" );
            isExist &= svcList[ idx ].contains( "fileFullPath" );
            isExist &= svcList[ idx ].contains( "terminateDefence" );
            isExist &= svcList[ idx ].contains( "update" );
            isExist &= svcList[ idx ].contains( "isStop" );
            isExist &= svcList[ idx ].contains( "runSystemProcess" );

            if( isExist == false )
                continue;

            SERVICE_ITEM item;

            item.sUniqueName = Shared::String::u82ws( svcList[ idx ][ "uniqueName" ].get<std::string>() );
            vecUniqueName.push_back( item.sUniqueName );
            item.sFileFullPath = Shared::String::u82ws( svcList[ idx ][ "fileFullPath" ].get<std::string>() );

            item.sFileFullPath = item.sFileFullPath.replace( "{currentPath}", Shared::File::GetCurrentPath() );
            item.isDefence = svcList[ idx ][ "terminateDefence" ].get<bool>();
            item.isUpdate = svcList[ idx ][ "update" ].get<bool>();
            item.isStop = svcList[ idx ][ "isStop" ].get<bool>();
            item.bRunSystemProcess = svcList[ idx ][ "runSystemProcess" ].get<bool>();

            if( _mapServiceItem.count( item.sUniqueName ) > 0 )
            {
                SERVICE_ITEM itemCompare = _mapServiceItem[ item.sUniqueName ];
                bool isChanged = false;

                if( item.sFileFullPath.compare( itemCompare.sFileFullPath ) != 0 )
                    isChanged = true;
                else if( item.isDefence != itemCompare.isDefence )
                    isChanged = true;
                else if( item.isUpdate != itemCompare.isUpdate )
                    isChanged = true;
                else if( item.isStop != itemCompare.isStop )
                    isChanged = true;
                else if( item.bRunSystemProcess != itemCompare.bRunSystemProcess )
                    isChanged = true;

                if( isChanged == false )
                    continue;

                {
                    std::lock_guard<std::mutex> lck( _lck );
                    _mapServiceItem.erase( item.sUniqueName );
                }
            }

            {
                std::lock_guard<std::mutex> lck( _lck );
                _mapServiceItem[ item.sUniqueName ] = std::move( item );
            }
        }

        std::vector<XString> vecCurrentList;
        for( auto item = _mapServiceItem.begin(); item != _mapServiceItem.end(); ++item )
        {
            vecCurrentList.push_back( item->first );
        }

        for( auto sCurrent : vecCurrentList )
        {
            bool isAlive = false;

            for( auto sPrev : vecUniqueName )
            {
                if( sPrev.compare( sCurrent ) == 0 )
                {
                    isAlive = true;
                    break;
                }
            }

            if( isAlive == false )
            {
                SERVICE_ITEM* item;

                if( _mapServiceItem.count( sCurrent ) > 0 )
                {
                    item = &_mapServiceItem[ sCurrent ];
                    item->isStop = true;
                }
                else
                {
                    continue;
                }
            }
        }

        std::vector<XString> vecDeleted;
        for( auto item = _mapServiceItem.begin(); item != _mapServiceItem.end(); ++item )
        {
            if( _mapServiceItemThread.count( item->second.sUniqueName ) > 0 )
            {
                if( item->second.isStop == false )
                    continue;

                _mapServiceItemThread[ item->second.sUniqueName ].join();
                _mapServiceItemThread.erase( item->second.sUniqueName );
                vecDeleted.push_back( item->second.sUniqueName );
                continue;
            }

            if( item->second.isStop == false )
                _mapServiceItemThread[ item->second.sUniqueName ] = std::thread( std::bind( &cSvcMgr::ServiceWorker, this, item->second.sUniqueName ) );
        }

        for( auto item : vecDeleted )
        {
            std::lock_guard<std::mutex> lck( _lck );
            _mapServiceItem.erase( item );
        }
    }
}
