#include "stdafx.h"

#include <functional>

#include "strsafe.h"

#include "ServiceMgr.hpp"
#include "Utils/SharedProcess.h"
#include "Utils/SharedFile.h"
#include "Windows/SharedWinTools.h"

#define T_SVC_NAME TEXT("ServiceMgr")

using namespace Shared;
SERVICE_STATUS          gSvcStatus;
SERVICE_STATUS_HANDLE   gSvcStatusHandle;
HANDLE                  ghSvcStopEvent = NULL;
std::thread             thSVCStop;

VOID SvcReportEvent( LPTSTR szFunction )
{
    HANDLE hEventSource;
    LPCTSTR lpszStrings[ 2 ];
    TCHAR Buffer[ 80 ];

    hEventSource = RegisterEventSource( NULL, T_SVC_NAME );

    if( NULL != hEventSource )
    {
        StringCchPrintf( Buffer, 80, TEXT( "%s failed with %d" ), szFunction, GetLastError() );

        lpszStrings[ 0 ] = T_SVC_NAME;
        lpszStrings[ 1 ] = Buffer;

        ReportEvent( hEventSource,        // event log handle
                     EVENTLOG_ERROR_TYPE, // event type
                     0,                   // event category
                     1,           // event identifier
                     NULL,                // no security identifier
                     2,                   // size of lpszStrings array
                     0,                   // no binary data
                     lpszStrings,         // array of strings
                     NULL );               // no binary data

        DeregisterEventSource( hEventSource );
    }
}

VOID ReportSvcStatus( DWORD dwCurrentState,
                      DWORD dwWin32ExitCode,
                      DWORD dwWaitHint )
{
    static DWORD dwCheckPoint = 1;

    // Fill in the SERVICE_STATUS structure.

    gSvcStatus.dwCurrentState = dwCurrentState;
    gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
    gSvcStatus.dwWaitHint = dwWaitHint;

    if( dwCurrentState == SERVICE_START_PENDING )
        gSvcStatus.dwControlsAccepted = 0;
    else gSvcStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;

    if( ( dwCurrentState == SERVICE_RUNNING ) ||
        ( dwCurrentState == SERVICE_STOPPED ) )
        gSvcStatus.dwCheckPoint = 0;
    else gSvcStatus.dwCheckPoint = dwCheckPoint++;

    // Report the status of the service to the SCM.
    SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}

void SVCStopthread()
{
    while( 1 )
    {
        // Check whether to stop the service.

        WaitForSingleObject( ghSvcStopEvent, INFINITE );

        ReportSvcStatus( SERVICE_STOPPED, NO_ERROR, 0 );
        return;
    }
}

VOID SvcInit( DWORD dwArgc, LPTSTR* lpszArgv )
{
    // TO_DO: Declare and set any required variables.
    //   Be sure to periodically call ReportSvcStatus() with 
    //   SERVICE_START_PENDING. If initialization fails, call
    //   ReportSvcStatus with SERVICE_STOPPED.

    // Create an event. The control handler function, SvcCtrlHandler,
    // signals this event when it receives the stop control code.

    ghSvcStopEvent = CreateEvent(
        NULL,    // default security attributes
        TRUE,    // manual reset event
        FALSE,   // not signaled
        NULL );   // no name

    if( ghSvcStopEvent == NULL )
    {
        ReportSvcStatus( SERVICE_STOPPED, GetLastError(), 0 );
        return;
    }

    // Report running status when initialization is complete.

    ReportSvcStatus( SERVICE_RUNNING, NO_ERROR, 0 );

    // TO_DO: Perform work until service stops.

    thSVCStop = std::thread( std::bind( &SVCStopthread ) );

#ifndef _DEBUG
    cSvcMgr a;
#endif
}

VOID WINAPI SvcCtrlHandler( DWORD dwCtrl )
{
    // Handle the requested control code. 

    switch( dwCtrl )
    {
        case SERVICE_CONTROL_STOP:
        ReportSvcStatus( SERVICE_STOP_PENDING, NO_ERROR, 0 );

        // Signal the service to stop.

        SetEvent( ghSvcStopEvent );
        ReportSvcStatus( gSvcStatus.dwCurrentState, NO_ERROR, 0 );

        return;

        case SERVICE_CONTROL_INTERROGATE:
        break;

        default:
        break;
    }

}

VOID WINAPI SvcMain( DWORD dwArgc, LPTSTR* lpszArgv )
{
    // Register the handler function for the service

    gSvcStatusHandle = RegisterServiceCtrlHandler(
        T_SVC_NAME,
        SvcCtrlHandler );

    if( !gSvcStatusHandle )
    {
        SvcReportEvent( ( LPTSTR )"RegisterServiceCtrlHandler" );
        return;
    }

    // These SERVICE_STATUS members remain as set here

    gSvcStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gSvcStatus.dwServiceSpecificExitCode = 0;

    // Report initial status to the SCM

    ReportSvcStatus( SERVICE_START_PENDING, NO_ERROR, 3000 );

    // Perform service-specific initialization and work.

    SvcInit( dwArgc, lpszArgv );
}

int WINAPI WinMain( __in HINSTANCE hInstance, __in HINSTANCE hPrevInstance, __in LPSTR lpCmdLine, __in int nCmdShow )
{
    Windows::SetPrivilege( SE_DEBUG_NAME, TRUE );
    XString sMgr = Shared::File::GetCurrentPath( true ) + "SvcStarter.exe";

    if( File::IsExistFile( sMgr ) == false )
        return 0;

    XString sJsonFile = File::GetCurrentPath( true ) + "List.json";

    if( File::IsExistFile( sJsonFile ) == false )
        return 0;

#ifndef _DEBUG
    if( Windows::IsExistService( T_SVC_NAME ) == false )
    {
        sMgr = Shared::Format::Format( "{} -i {} \"{}\"", sMgr, T_SVC_NAME, File::CurrentPathAppend( "ServiceMgr.exe" ) );
        Process::ExecutionProcess( sMgr );
        return 0;
    }

    SERVICE_TABLE_ENTRY STE[] =
    {
        {( WCHAR* )T_SVC_NAME, ( LPSERVICE_MAIN_FUNCTION )SvcMain},
        {NULL,NULL}
    };

    StartServiceCtrlDispatcher( STE );
#else
    cSvcMgr a;
#endif

    return 0;
}
