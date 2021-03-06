// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com

/*
This file is part of Date Fix Tool - A tool for correcting the system date on a computer with the old BIOS.

Copyright (C) 2021 Nikolay Raspopov <raspopov@cherubicsoft.com>

https://github.com/raspopov/DateFixTool

This program is free software : you can redistribute it and / or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
( at your option ) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.If not, see < http://www.gnu.org/licenses/>.
*/

#define STRICT
#define WIN32_LEAN_AND_MEAN

#define _WIN32_WINNT 0x0501	// for Windows XP
#include <SDKDDKVer.h>

#include <windows.h>
#include <tchar.h>

#include <Shlwapi.h>
#pragma comment(lib, "Shlwapi.lib")

#include "resource.h"
#include "DateFixToolEvents.h"

#define SVCNAME			_T("DateFixTool")
#define SVCDISPLAY		_T("Date Fix Tool")
#define SVCDESC			_T("A tool for correcting the system date on a computer with the old BIOS.")
#define EVENTSRC		_T("SYSTEM\\CurrentControlSet\\services\\eventlog\\Application\\") SVCNAME

SERVICE_STATUS          gSvcStatus = { SERVICE_WIN32_OWN_PROCESS };
SERVICE_STATUS_HANDLE   gSvcStatusHandle = nullptr;
HANDLE                  ghSvcStopEvent = nullptr;

BOOL SvcInstall(LPCTSTR szServiceName, LPCTSTR szServiceDisplay);
BOOL SvcUnInstall(LPCTSTR szServiceName);

void WINAPI SvcCtrlHandler(DWORD dwCtrl);
void WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv);
void ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode = NO_ERROR, DWORD dwWaitHint = 3000);
void SvcReportEvent(WORD type /* EVENTLOG_ERROR_TYPE, EVENTLOG_INFORMATION_TYPE, EVENTLOG_WARNING_TYPE */, LPCTSTR msg);


int __cdecl _tmain(int argc, TCHAR *argv[])
{
	HANDLE hAppMutex = CreateMutex( nullptr, TRUE, _T("Global\\") SVCNAME );

	if ( argc > 1 && lstrcmpi( argv[ 1 ], _T("install") ) == 0 )
    {
        if ( SvcInstall( SVCNAME, SVCDISPLAY ) )
		{
			_tprintf( _T("Service installed successfully\n") );
		}
		else
		{
			_tprintf( _T("Cannot install service (%u)\n"), GetLastError() );
		}
    }
	else if ( argc > 1 && lstrcmpi( argv[ 1 ], _T("uninstall") ) == 0 )
    {
		if ( SvcUnInstall( SVCNAME ) )
		{
			_tprintf( _T("Service uninstalled successfully\n") );
		}
		else
		{
			_tprintf( _T("Cannot uninstall service (%u)\n"), GetLastError() );
		}
    }
	else
	{
		TCHAR SvcName[] = { SVCNAME };
		const SERVICE_TABLE_ENTRY DispatchTable[] =
		{
			{ SvcName, SvcMain },
			{ nullptr, nullptr }
		};

		if ( ! StartServiceCtrlDispatcher( DispatchTable ) )
		{
			if ( GetLastError() == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT )
			{
				_tprintf( _T("%s\nUsage: %s [ install | uninstall ]\n"), SVCDESC, PathFindFileName( argv[ 0 ] ) );
			}
			else
			{
				SvcReportEvent( EVENTLOG_ERROR_TYPE, _T("StartServiceCtrlDispatcher") );
			}
		}
	}

	// Don't CloseHandle( hAppMutex );
	hAppMutex;

	return 0;
}

BOOL SvcInstall(LPCTSTR szServiceName, LPCTSTR szServiceDisplay)
{
	BOOL success = FALSE;

	TCHAR szPath[ MAX_PATH ];
    if ( GetModuleFileName( nullptr, szPath, MAX_PATH ) )
    {
		// Register event source
		DWORD types = EVENTLOG_ERROR_TYPE | EVENTLOG_INFORMATION_TYPE | EVENTLOG_WARNING_TYPE;
		SHSetValue( HKEY_LOCAL_MACHINE, EVENTSRC, _T("TypesSupported"), REG_DWORD, &types, sizeof( types ) );
		SHRegSetPath( HKEY_LOCAL_MACHINE, EVENTSRC, _T("EventMessageFile"), szPath, 0 );

		if ( SC_HANDLE schSCManager = OpenSCManager( nullptr,  nullptr, SC_MANAGER_ALL_ACCESS ) )
		{
			SC_HANDLE schService = OpenService( schSCManager, szServiceName, SERVICE_ALL_ACCESS );
			if ( ! schService )
			{
				// Creating a new service
				schService = CreateService( schSCManager, szServiceName, szServiceDisplay, SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
					SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, szPath, nullptr, nullptr, nullptr, nullptr, nullptr );
			}
			if ( schService )
			{
				success = TRUE;

				// Set service description
				TCHAR desc[ MAX_PATH ];
				_tcscpy_s( desc, SVCDESC );
				SERVICE_DESCRIPTION srv_desc = { desc };
				ChangeServiceConfig2( schService, SERVICE_CONFIG_DESCRIPTION, &srv_desc );

				// Start it
				StartService( schService, 0, nullptr );

				CloseServiceHandle( schService );
			}

			CloseServiceHandle( schSCManager );
		}
	}

	return success;
}

BOOL SvcUnInstall(LPCTSTR szServiceName)
{
	BOOL success = FALSE;

	SHDeleteKey( HKEY_LOCAL_MACHINE, EVENTSRC );

	if ( SC_HANDLE schSCManager = OpenSCManager( nullptr,  nullptr, SC_MANAGER_ALL_ACCESS ) )
	{
		if ( SC_HANDLE schService = OpenService( schSCManager, szServiceName, SERVICE_ALL_ACCESS ) )
		{
			// Stop it
			SERVICE_STATUS status = {};
			ControlService( schService, SERVICE_CONTROL_STOP, &status );

			// Delete it
			success = DeleteService( schService );

			CloseServiceHandle( schService );
		}

		CloseServiceHandle( schSCManager );
	}

	return success;
}

void WINAPI SvcMain(DWORD dwArgc, LPTSTR *lpszArgv)
{
	TCHAR buf[ 80 ];

	dwArgc;
	lpszArgv;

	gSvcStatusHandle = RegisterServiceCtrlHandler( SVCNAME, SvcCtrlHandler );
	if( ! gSvcStatusHandle )
	{
		SvcReportEvent( EVENTLOG_ERROR_TYPE, _T("RegisterServiceCtrlHandler") );
		return;
	}

	ReportSvcStatus( SERVICE_START_PENDING );

	ghSvcStopEvent = CreateEvent( nullptr, TRUE,  FALSE, nullptr );
	if ( ghSvcStopEvent )
	{
		SYSTEMTIME st = {};
		GetSystemTime( &st );
		if ( st.wYear < 2020 )
		{
			// Setting a good date
			SYSTEMTIME good = st;
			if ( good.wYear < 2000 )
			{
				good.wYear = good.wYear % 100 + 2000;
			}
			else
			{
				good.wYear += 20;
			}
			_stprintf_s( buf, _T("Setting a good date: %02u-%02u-%04u -> %02u-%02u-%04u"), st.wDay, st.wMonth, st.wYear, good.wDay, good.wMonth, good.wYear );
			SvcReportEvent( EVENTLOG_INFORMATION_TYPE, buf );
			if ( SetSystemTime( &good ) )
			{
				// Waiting for shutdown (or stop)
				ReportSvcStatus( SERVICE_RUNNING );
				WaitForSingleObject( ghSvcStopEvent, INFINITE );

				// Stopped
				ReportSvcStatus( SERVICE_STOP_PENDING );

				GetSystemTime( &st );
				if ( st.wYear >= 2020 )
				{
					// Setting a fake date
					SYSTEMTIME fixed = st;
					fixed.wYear -= 20;
					_stprintf_s( buf, _T("Setting a fixed date: %02u-%02u-%04u -> %02u-%02u-%04u"), st.wDay, st.wMonth, st.wYear, fixed.wDay, fixed.wMonth, fixed.wYear );
					SvcReportEvent( EVENTLOG_INFORMATION_TYPE, buf );
					if ( ! SetSystemTime( &fixed ) )
					{
						SvcReportEvent( EVENTLOG_ERROR_TYPE, _T("SetSystemTime") );
					}
				}
				else
				{
					_stprintf_s( buf, _T("Date is already fixed: %02u-%02u-%04u"), st.wDay, st.wMonth, st.wYear );
					SvcReportEvent( EVENTLOG_INFORMATION_TYPE, buf );
				}
			}
			else
			{
				SvcReportEvent( EVENTLOG_ERROR_TYPE, _T("SetSystemTime") );
			}
		}
		else
		{
			_stprintf_s( buf, _T("Date is already correct: %02u-%02u-%04u"), st.wDay, st.wMonth, st.wYear );
			SvcReportEvent( EVENTLOG_INFORMATION_TYPE, buf );
		}
	}

	ReportSvcStatus( SERVICE_STOPPED );
}

void ReportSvcStatus(DWORD dwCurrentState, DWORD dwWin32ExitCode, DWORD dwWaitHint)
{
    static DWORD dwCheckPoint = 1;

	gSvcStatus.dwCurrentState = dwCurrentState;
	gSvcStatus.dwWin32ExitCode = dwWin32ExitCode;
	gSvcStatus.dwWaitHint = dwWaitHint;
	gSvcStatus.dwControlsAccepted = ( ( dwCurrentState == SERVICE_START_PENDING ) ? 0 : SERVICE_ACCEPT_STOP ) | SERVICE_ACCEPT_SHUTDOWN;
	gSvcStatus.dwCheckPoint = ( ( dwCurrentState == SERVICE_RUNNING) || ( dwCurrentState == SERVICE_STOPPED ) ) ? 0 : ( dwCheckPoint ++ );

    SetServiceStatus( gSvcStatusHandle, &gSvcStatus );
}

void WINAPI SvcCtrlHandler(DWORD dwCtrl)
{
	switch( dwCtrl )
	{
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
		ReportSvcStatus( SERVICE_STOP_PENDING );
        SetEvent( ghSvcStopEvent );
        break;
	}
}

void SvcReportEvent(WORD type /* EVENTLOG_ERROR_TYPE, EVENTLOG_INFORMATION_TYPE, EVENTLOG_WARNING_TYPE */, LPCTSTR msg)
{
	if ( HANDLE hEventSource = RegisterEventSource( nullptr, SVCNAME ) )
	{
		TCHAR buf[ 80 ];

		DWORD code = SVC_INFO;
		if ( type == EVENTLOG_ERROR_TYPE )
		{
			code = SVC_ERROR;
			_stprintf_s( buf, _countof( buf ), _T("%s failed with error code: %u"), msg, GetLastError() );
		}
		else
		{
			_tcscpy_s( buf, msg );
		}

		LPCTSTR strings[] = { SVCNAME, buf }; // %1, %2, ...
		ReportEvent( hEventSource, type, 0, code, nullptr, _countof( strings ), 0, strings, nullptr );

		DeregisterEventSource( hEventSource );
	}
}
