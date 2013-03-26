/* -------------------------------------------------------------------------------------------------
 * ConfigReader.h - header file for CConfigReader class that is responsible for reading 
 * configuration for launching the specified file from the system registry.
 *
 * Quick Launch (qlaunch) utility for Windows Vista x86/x64
 * Copyright (c) 2009 Sergey Rybalkin - rybalkinsp@gmail.com
 ------------------------------------------------------------------------------------------------ */
#include "StdAfx.h"
#pragma once

class CConfigReader
{
public :
    CConfigReader ( LPCTSTR lpszItemName ) ;
    ~CConfigReader ( ) ;

    BOOL Parse ( SHELLEXECUTEINFO* pExecInfo ) ;

private :
    BOOL ReadRegSZ ( HKEY hKey , LPCTSTR lpszValueName , TCHAR** ppBuf ) ;
    VOID PopulateExecuteInformation ( SHELLEXECUTEINFO* pExecInfo ) ;

    LPTSTR m_lpszItemName ;

    // Fields below should be cleaned up in destructor
    TCHAR* m_lpszPath ;
    TCHAR* m_lpszParams ;
    DWORD  m_dwMonitorIndex ;
    DWORD  m_dwWindowState ;
};