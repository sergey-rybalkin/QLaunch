/* -------------------------------------------------------------------------------------------------
 * DirectoryLister.cpp - implementation file for CDirectoryLister class that is responsible for 
 * showing a popup menu with the content of the specified directory and launching selected files.
 *
 * Quick Launch (qlaunch) utility for Windows Vista x86/x64
 * Copyright (c) 2009 Sergey Rybalkin - rybalkinsp@gmail.com
 ------------------------------------------------------------------------------------------------ */
#include "StdAfx.h"
#include "DirectoryLister.h"

CDirectoryLister::CDirectoryLister ( HWND hwndParent )
{
    m_hwndParent = hwndParent ;
    m_nCreatedMenusCount = 0 ;
    m_pMenus = NULL ;
}

CDirectoryLister::~CDirectoryLister ( )
{
    for ( USHORT index = 0 ; index < m_nCreatedMenusCount ; index++ )
        ::DestroyMenu ( m_pMenus [ index ] ) ;

    if ( NULL != m_pMenus )
        delete[] m_pMenus ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Shows popup menu build using the content of the specified directory. Only 1 level depth is
// supported (subdirectories of subdirectories will not be expanded)
INT CDirectoryLister::ShowMenu ( LPCTSTR lpszDirectoryToList )
{
    // Find out how many subdirectories are there in our root directory and allocate memory for our 
    // submenus
    USHORT nSubdirectoriesCount = GetSubdirectoriesCount ( lpszDirectoryToList ) ;
    if ( -1 == nSubdirectoriesCount )
        return -1 ;
    m_nCreatedMenusCount = nSubdirectoriesCount + 1 ; // reserve one item for root menu

    m_pMenus = new HMENU [ m_nCreatedMenusCount ] ;
    if ( NULL == m_pMenus )
        return -1 ;
    for ( USHORT index = 0 ; index < m_nCreatedMenusCount ; index++ )
    {
        m_pMenus [ index ] = CreatePopupMenu ( ) ;
        if ( NULL == m_pMenus [ index ] )
            return -1 ;
    }
    
    // Search for subdirectories in our root folder and add them to the menu as submenus
    DWORD nFilesFound = ListSubdirectories ( lpszDirectoryToList ) ;
    if ( -1 == nFilesFound )
        return -1 ;

    // Search for files in our root directory

    WIN32_FIND_DATA fdFileInformation ;
    HANDLE hFind ;
    TCHAR szSearchPattern [ MAX_PATH ] ;

    // This search pattern will return all files
    HRESULT hr = StringCchPrintf ( szSearchPattern , 
                                   MAX_PATH , 
                                   TEXT ( "%s\\*" ) , 
                                   lpszDirectoryToList ) ;
    if ( FAILED ( hr ) )
        return -1 ;

    hFind = FindFirstFile ( szSearchPattern , &fdFileInformation ) ;
    if ( NULL == hFind )
        return -1 ;

    do
    {
        // We are interested only in non-system directories and files
        if ( ( '.' == fdFileInformation.cFileName [ 0 ] ) ||
             ( fdFileInformation.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ) ||
             ( fdFileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            continue ;

        // Append menu item that will point to the file found
        BOOL bRetVal = AppendMenu ( m_pMenus [ 0 ] , 
                                    MF_STRING , 
                                    WM_USER + nFilesFound , 
                                    fdFileInformation.cFileName ) ;
        if ( !bRetVal )
            return -1 ;

        // And store relative file path in our global storage for found files for future use
        m_ppszFolderContent [ nFilesFound ] = ( TCHAR* ) HeapAlloc ( GetProcessHeap ( ) , 
                                                                     0 , 
                                                                     MAX_PATH * sizeof ( TCHAR ) ) ;
        if ( NULL == m_ppszFolderContent [ nFilesFound ] )
            return FALSE ;

        hr = StringCchCopy ( m_ppszFolderContent [ nFilesFound ] , 
                             MAX_PATH , 
                             fdFileInformation.cFileName ) ; 
        if ( FAILED ( hr ) )
            return FALSE ;

        nFilesFound ++ ;
    }
    while ( FindNextFile ( hFind , &fdFileInformation ) != 0 );

    if ( !FindClose ( hFind ) )
        return -1 ;

    POINT pt ;
    if ( !GetCursorPos ( &pt ) )
    {
        pt.x = 0 ;
        pt.y = 0 ;
    }

    TrackPopupMenu ( m_pMenus [ 0 ] , 
                     TPM_CENTERALIGN | TPM_VCENTERALIGN , 
                     pt.x , 
                     pt.y , 
                     0 , 
                     m_hwndParent , 
                     NULL ) ;

    return nFilesFound ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Opens file listed in the popup menu under the specified index
BOOL CDirectoryLister::OpenFile ( LPCTSTR lpszRootDirectory, DWORD dwIndex )
{
    TCHAR szFilePath [ MAX_PATH ] ;
    HRESULT hr = StringCchPrintf ( szFilePath , 
                                   MAX_PATH , 
                                   TEXT ( "%s\\%s" ) , 
                                   lpszRootDirectory , 
                                   m_ppszFolderContent [ dwIndex ] ) ;

    if ( FAILED ( hr ) )
        return FALSE ;

    return ( (INT_PTR) ShellExecute ( 
        m_hwndParent , TEXT ( "open" ) , szFilePath , NULL , NULL , SW_SHOWNORMAL ) > 32 ) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Adds content of all subdirectories of the specified root directory as submenus to the root menu 
// item and returns number of files/directories listed.
INT CDirectoryLister::ListSubdirectories ( LPCTSTR lpszRootDirectory )
{
    WIN32_FIND_DATA fdFileInformation ;
    HANDLE hFind ;
    TCHAR szSearchPattern [ MAX_PATH ] ;
    USHORT nFilesFound = 0 ;
    USHORT nMenuIndex = 1 ; // menu under index 0 is reserved for the root menu

    // This search pattern will return all files
    HRESULT hr = StringCchPrintf ( szSearchPattern , 
                                   MAX_PATH , 
                                   TEXT ( "%s\\*" ) , 
                                   lpszRootDirectory ) ;
    if ( FAILED ( hr ) )
        return -1 ;

    hFind = FindFirstFile ( szSearchPattern , &fdFileInformation ) ;
    if ( NULL == hFind )
        return -1 ;

    do
    {
        // We are interested only in non-system directories, ignore . .. .svn and others
        if ( ( '.' != fdFileInformation.cFileName [ 0 ] ) &&
             ( fdFileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
        {
            BOOL bRetVal = PopulateDirectoryMenu ( lpszRootDirectory , 
                                                   fdFileInformation.cFileName , 
                                                   nMenuIndex++ , 
                                                   &nFilesFound ) ;
            if ( !bRetVal )
                return -1 ;
            bRetVal = AppendMenu ( m_pMenus [ 0 ] , 
                                   MF_POPUP , 
                                   ( UINT_PTR ) m_pMenus [ nMenuIndex -1 ] , 
                                   fdFileInformation.cFileName ) ;
            if ( !bRetVal )
                return -1 ;
        }
    }
    while ( FindNextFile ( hFind , &fdFileInformation ) != 0 );

    if ( !FindClose ( hFind ) )
        return -1 ;

    return nFilesFound ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Adds all files and subdirectories in the specified directory to the menu with specified index as 
// items.
BOOL CDirectoryLister::PopulateDirectoryMenu ( LPCTSTR lpszRootDirectory ,
                                               LPCTSTR lpszSubDirectory , 
                                               USHORT nMenuIndex , 
                                               USHORT* pnNextItemIndex )
{
    WIN32_FIND_DATA fdFileInformation ;
    HANDLE hFind ;
    TCHAR szSearchPattern [ MAX_PATH ] ;

    // This search pattern will return all files
    HRESULT hr = StringCchPrintf ( szSearchPattern , 
                                   MAX_PATH , 
                                   TEXT ( "%s\\%s\\*" ) , 
                                   lpszRootDirectory ,
                                   lpszSubDirectory ) ;
    if ( FAILED ( hr ) )
        return -1 ;

    hFind = FindFirstFile ( szSearchPattern , &fdFileInformation ) ;
    if ( NULL == hFind )
        return -1 ;

    do
    {
        // We are interested only in non-system directories and files
        if ( ( '.' == fdFileInformation.cFileName [ 0 ] ) ||
             ( fdFileInformation.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM ) )
            continue ;

        // Append menu item that will point to the file found
        BOOL bRetVal = AppendMenu ( m_pMenus [ nMenuIndex ] , 
                                    MF_STRING , 
                                    WM_USER + *pnNextItemIndex , 
                                    fdFileInformation.cFileName ) ;
        if ( !bRetVal )
            return FALSE ;

        // And store relative file path in our global storage for found files for future use
        m_ppszFolderContent [ *pnNextItemIndex ] = ( TCHAR* ) HeapAlloc ( 
            GetProcessHeap ( ) , 0 , MAX_PATH * sizeof ( TCHAR ) ) ;
        if ( NULL == m_ppszFolderContent [ *pnNextItemIndex ] )
            return FALSE ;

        hr = StringCchPrintf ( m_ppszFolderContent [ *pnNextItemIndex ] , 
                               MAX_PATH , 
                               TEXT ( "%s\\%s" ) , 
                               lpszSubDirectory , 
                               fdFileInformation.cFileName ) ;
        if ( FAILED ( hr ) )
            return FALSE ;

        (*pnNextItemIndex)++ ;
    }
    while ( FindNextFile ( hFind , &fdFileInformation ) != 0 );

    return FindClose ( hFind ) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Returns number of subdirectories in the specified directory.
SHORT CDirectoryLister::GetSubdirectoriesCount ( LPCTSTR lpszDirectory )
{
    // The fastest way to count subdirectories in the specified directory is to search for all files
    // and count directories among them
    WIN32_FIND_DATA fdFileInformation ;
    HANDLE hFind ;
    TCHAR szSearchPattern [ MAX_PATH ] ;
    USHORT usRetVal = 0 ;

    // This search pattern will return all files
    HRESULT hr = StringCchPrintf ( szSearchPattern , MAX_PATH , TEXT ( "%s\\*" ) , lpszDirectory ) ;
    if ( FAILED ( hr ) )
        return -1 ;

    hFind = FindFirstFile ( szSearchPattern , &fdFileInformation ) ;
    if ( NULL == hFind )
        return -1 ;

    do
    {
        // We are interested only in non-system directories, ignore . .. .svn and others
        if ( ( '.' != fdFileInformation.cFileName [ 0 ] ) && 
             ( fdFileInformation.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ) )
            usRetVal ++ ;
    }
    while ( FindNextFile ( hFind , &fdFileInformation ) != 0 );

    if ( !FindClose ( hFind ) )
        return -1 ;

    return usRetVal ;
}