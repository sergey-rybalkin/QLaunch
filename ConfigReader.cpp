/* -------------------------------------------------------------------------------------------------
 * ConfigReader.cpp - implementation file for CConfigReader class that is responsible for reading 
 * configuration for launching the specified file from the system registry.
 *
 * Quick Launch (qlaunch) utility for Windows Vista x86/x64
 * Copyright (c) 2009 Sergey Rybalkin - rybalkinsp@gmail.com
 ------------------------------------------------------------------------------------------------ */

#include "StdAfx.h"
#include "ConfigReader.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Globals
TCHAR g_szExecutorRegKey    [ ] = TEXT ( "SOFTWARE\\LiveKeys\\Executor" ) ;
TCHAR g_szPathRegVal        [ ] = TEXT ( "Path" ) ;
TCHAR g_szParamsRegVal      [ ] = TEXT ( "Params" ) ;
TCHAR g_szMonitorRegVal     [ ] = TEXT ( "Monitor" ) ;
TCHAR g_szWindowStateRegVal [ ] = TEXT ( "WindowState" ) ;
TCHAR g_szShellExecuteVerb  [ ] = TEXT ( "open" ) ;

struct FINDMONITOR
{
    DWORD dwRequiredIndex ;
    DWORD dwCurrentIndex ;
    HMONITOR hMonitor ;
};

BOOL CALLBACK EnumMonitorsCallBack ( HMONITOR hMonitor, HDC , LPRECT , LPARAM dwData )
{
    FINDMONITOR* pFindMonitor = (FINDMONITOR*)dwData;
    if ( pFindMonitor->dwCurrentIndex == pFindMonitor->dwRequiredIndex )
    {
        pFindMonitor->hMonitor = hMonitor ;
        return FALSE ;
    }
    
    pFindMonitor->dwCurrentIndex++;

    return TRUE;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CConfigReader::CConfigReader ( LPCTSTR lpszItemName )
{
    m_lpszItemName = ( LPTSTR ) lpszItemName ;
    m_lpszParams = NULL ;
    m_lpszPath = NULL ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
CConfigReader::~CConfigReader ( )
{
    if ( NULL != m_lpszParams )
        delete [ ] m_lpszParams ;
    if ( NULL != m_lpszPath )
        delete [ ] m_lpszPath ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Reads configuration of the specified quick launch item and fills the specified SHELLEXECUTEINFO
// structure with information required to launch item.
BOOL CConfigReader::Parse ( SHELLEXECUTEINFO* pExecInfo )
{
    // Try to find quick launch item configuration registry key
    HKEY  hItemConfig ;
    TCHAR szConfigKey [ MAX_PATH ] ;
    HRESULT hr = StringCchPrintf ( szConfigKey , 
                                   MAX_PATH , 
                                   TEXT ( "%s\\%s" ) , 
                                   g_szExecutorRegKey , 
                                   m_lpszItemName ) ;
    if ( FAILED ( hr ) )
        return FALSE ;

	LONG lRetVal = RegOpenKeyEx ( HKEY_CURRENT_USER , 
                                  szConfigKey , 
                                  0 , 
                                  KEY_READ , 
                                  &hItemConfig ) ;
    if ( ERROR_SUCCESS != lRetVal )
        return FALSE ;
    
    // Once configuration key was found let's read configuration into pExecInfo
    if ( !ReadRegSZ ( hItemConfig , g_szPathRegVal , &m_lpszPath ) ) // Path
        return FALSE ;

    if ( !ReadRegSZ ( hItemConfig , g_szParamsRegVal , &m_lpszParams ) ) // Command line parameters
        return FALSE ;

    DWORD dwBufSize = sizeof ( DWORD ) ;
    lRetVal = RegGetValue ( hItemConfig , 
                            NULL , 
                            g_szMonitorRegVal ,
                            RRF_RT_REG_DWORD ,
                            NULL ,
                            &m_dwMonitorIndex ,
                            &dwBufSize ) ;
    if ( ERROR_SUCCESS != lRetVal )
        return FALSE ;
    lRetVal = RegGetValue ( hItemConfig , 
                            NULL , 
                            g_szWindowStateRegVal ,
                            RRF_RT_REG_DWORD ,
                            NULL ,
                            &m_dwWindowState ,
                            &dwBufSize ) ;
    if ( ERROR_SUCCESS != lRetVal )
        return FALSE ;
    

    PopulateExecuteInformation ( pExecInfo ) ;

    return TRUE ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Fills specified SHELLEXECUTEINFO structure with the information read from registry
VOID CConfigReader::PopulateExecuteInformation ( SHELLEXECUTEINFO* pExecInfo )
{
    pExecInfo->cbSize = sizeof ( SHELLEXECUTEINFO ) ;
    pExecInfo->fMask = 
        SEE_MASK_FLAG_DDEWAIT | SEE_MASK_HMONITOR | SEE_MASK_NOASYNC | SEE_MASK_UNICODE ;
    pExecInfo->hwnd = NULL ;
    pExecInfo->lpVerb = g_szShellExecuteVerb ;
    pExecInfo->lpFile = m_lpszPath ;
    pExecInfo->lpParameters = m_lpszParams ;
    pExecInfo->lpDirectory = NULL ;
    pExecInfo->nShow = m_dwWindowState ;

    // Set monitor handle. 0 means primary monitor, if index is greater then zero we will enumerate
    // through all monitors and find the required one using index. 
    if ( 0 < m_dwMonitorIndex )
    {
        FINDMONITOR FindMonitor ;
        FindMonitor.dwCurrentIndex = 0 ;
        FindMonitor.dwRequiredIndex = m_dwMonitorIndex ;
        FindMonitor.hMonitor = NULL ;
        EnumDisplayMonitors ( NULL , NULL , EnumMonitorsCallBack , ( LPARAM ) &FindMonitor ) ;
        if ( NULL != FindMonitor.hMonitor )
        {
            pExecInfo->hMonitor = FindMonitor.hMonitor ;
            return ;
        }
    } // If index is invalid or is 0 we will default to primary monitor
    else
    {
        POINT ptZero = { 0 } ;
        pExecInfo->hMonitor = MonitorFromPoint ( ptZero , MONITOR_DEFAULTTOPRIMARY ) ;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Allocates required amount of memory and reads registry key value into ppBuf
BOOL CConfigReader::ReadRegSZ ( HKEY hKey , LPCTSTR lpszValueName , TCHAR** ppBuf )
{
    // Find out the size of the buffer required to store the value
    DWORD dwBufSize = 0 ;
    LONG lRetVal = RegGetValue ( hKey , 
                                 NULL , 
                                 lpszValueName , 
                                 RRF_RT_REG_SZ ,
                                 NULL ,
                                 NULL ,
                                 &dwBufSize ) ;
    if ( ERROR_SUCCESS != lRetVal )
        return FALSE ;

    // If value is not empty - allocate buffer and read value
    if ( dwBufSize > 0 )
    {
        *ppBuf = new TCHAR [ dwBufSize ] ;
        if ( NULL == *ppBuf )
            return FALSE ;
        lRetVal = RegGetValue ( hKey , 
                                NULL , 
                                lpszValueName , 
                                RRF_RT_REG_SZ ,
                                NULL ,
                                *ppBuf ,
                                &dwBufSize ) ;
        if ( ERROR_SUCCESS != lRetVal )
            return FALSE ;
    }

    return TRUE ;
}