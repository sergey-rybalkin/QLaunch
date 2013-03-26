/* -------------------------------------------------------------------------------------------------
 * DirectoryLister.h - header file for CDirectoryLister class that is responsible for showing a 
 * popup menu with the content of the specified directory and launching selected files.
 *
 * Quick Launch (qlaunch) utility for Windows Vista x86/x64
 * Copyright (c) 2009 Sergey Rybalkin - rybalkinsp@gmail.com
 ------------------------------------------------------------------------------------------------ */
#include "StdAfx.h"
#pragma once

#define NUM_MAX_ELEMENTS 100 // Defines maximum number of elements to be shown in our popup menu

class CDirectoryLister
{
public :
    CDirectoryLister ( HWND hwndParent ) ;
    ~CDirectoryLister ( ) ;

    // Shows popup menu with the content of the specified directory and ability to open any file by 
    // selecting menu item. Returns number of items listed in the specified menu.
    INT ShowMenu ( LPCTSTR lpszDirectoryToList ) ;

    // Runs ShellExecute function against the file listed in the opened menu under the specified 
    // index.
    BOOL OpenFile ( LPCTSTR lpszRootDirectory, DWORD dwIndex ) ;

private :
    SHORT GetSubdirectoriesCount ( LPCTSTR lpszDirectory ) ;
    INT  ListSubdirectories ( LPCTSTR lpszRootDirectory ) ;
    BOOL   PopulateDirectoryMenu ( LPCTSTR lpszRootDirectory ,
                                   LPCTSTR lpszSubDirectory , 
                                   USHORT nMenuIndex ,
                                   USHORT* pnNextItemIndex ) ;

    HWND   m_hwndParent ;
    HMENU* m_pMenus ;
    USHORT m_nCreatedMenusCount ;
    TCHAR* m_ppszFolderContent [ NUM_MAX_ELEMENTS ] ;
};