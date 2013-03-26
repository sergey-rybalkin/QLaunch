/* -------------------------------------------------------------------------------------------------
 * QuickLaunch.h - Defines the entry point for the application. Application is designer to speed up 
 * launching of applications and files while working with windows. Depending on the command line 
 * switches it can list the content of the specified directory in the popup menu and let user launch
 * any of the files in this directory or open specified file/application using settings stored in 
 * the system registry.
 *
 * Quick Launch (qlaunch) utility for Windows Vista x86/x64
 * Copyright (c) 2009 Sergey Rybalkin - rybalkinsp@gmail.com
 ------------------------------------------------------------------------------------------------ */
#include "stdafx.h"
#include "QuickLaunch.h"
#include "DirectoryLister.h"
#include "ConfigReader.h"

////////////////////////////////////////////////////////////////////////////////////////////////////
// Constants definition
#define WINDOWCLASS_NAME L"QLAUNCH"
#define SHUTD0WN_TIMER   1
#define SHUTD0WN_TIMEOUT 30000 // after 30 seconds application will exit no matter what
#define MENU_POS_X 960
#define MENU_POS_Y 600

////////////////////////////////////////////////////////////////////////////////////////////////////
// Function prototypes
LRESULT CALLBACK WindowProc ( HWND , UINT , WPARAM , LPARAM ) ;
BOOL ParseCommandLine ( LPCTSTR lpCmdLine ) ;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Global variables
HINSTANCE g_hInst ;
HWND      g_hWnd ;
CDirectoryLister* g_pLister ;
TCHAR g_szTargetDir [ MAX_PATH ] ;

////////////////////////////////////////////////////////////////////////////////////////////////////
// Application entry point
int APIENTRY _tWinMain ( HINSTANCE hInstance ,
                         HINSTANCE hPrevInstance ,
                         LPTSTR    lpCmdLine ,
                         int       nCmdShow )
{
    UNREFERENCED_PARAMETER ( hPrevInstance ) ;
    UNREFERENCED_PARAMETER ( nCmdShow ) ;
    
    g_hInst = hInstance ;

    // Create a fake window to listen to events.
    WNDCLASSEX wclx =  { 0 } ;
    wclx.cbSize         = sizeof( wclx ) ;
    wclx.lpfnWndProc    = &WindowProc ;
    wclx.hInstance      = hInstance ;
    wclx.lpszClassName  = WINDOWCLASS_NAME ;
    RegisterClassEx ( &wclx ) ;
    g_hWnd = CreateWindow ( WINDOWCLASS_NAME ,
                            0 ,
                            0 ,
                            0 ,
                            0 ,
                            0 ,
                            0 ,
                            HWND_MESSAGE ,
                            0 ,
                            g_hInst ,
                            0 ) ;

    // Create timer that will fire in 30 seconds to ensure that application will exit even if 
    // tracked popup menu will not be used.
    UINT_PTR uiRetVal = SetTimer ( g_hWnd , SHUTD0WN_TIMER , SHUTD0WN_TIMEOUT , NULL ) ;
    if ( NULL == uiRetVal )
    {
        MessageBox ( g_hWnd ,
                     TEXT ( "Could not create exit timer, application will terminate now" ) ,
                     TEXT ( "Quick Launch Menu - Initialization error" ) ,
                     MB_OK | MB_ICONERROR ) ;
        return 1 ;
    }

    if ( !ParseCommandLine ( lpCmdLine ) )
    {
        DestroyWindow ( g_hWnd ) ;
        UnregisterClass ( WINDOWCLASS_NAME , hInstance ) ;
 
        return 0 ;
    }

    // And finally start message handling loop.
    MSG msg ;
    while ( GetMessage ( &msg , NULL , 0 , 0 ) )
    {
        TranslateMessage ( &msg ) ;
        DispatchMessage ( &msg ) ;
    }

    DestroyWindow ( g_hWnd ) ;
    UnregisterClass ( WINDOWCLASS_NAME , hInstance ) ;
 
    return 0 ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Handles all events that we will receive
LRESULT CALLBACK WindowProc ( HWND hWnd , UINT uMsg , WPARAM wParam , LPARAM lParam )
{
    switch ( uMsg )
    {
        case WM_COMMAND :
            if ( wParam >= WM_USER )
            {
                g_pLister->OpenFile ( g_szTargetDir , ( DWORD ) wParam - WM_USER ) ;
                PostQuitMessage ( 0 ) ;
            }
            return 0 ;

        case WM_TIMER :
            if ( SHUTD0WN_TIMER == wParam )
                PostQuitMessage ( 0 ) ;
            return 0 ;

        case WM_CLOSE :
            PostQuitMessage ( 0 ) ;
            return 0 ;

        default :
            return DefWindowProc ( hWnd , uMsg , wParam , lParam ) ;
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////
VOID Usage ( )
{
    TCHAR szHelp [ 1024 ] ;
    StringCchPrintf ( szHelp , 
                      1024 , 
                      TEXT ( "%s\n%s\n%s\n\n%s\n%s" ) ,
                      TEXT ( "Quick Launch v1.0 - files/applications launching utility" ) ,
                      TEXT ( "Copyright (C) 2001-2009 Sergey Rybalkin" ) ,
                      TEXT ( "Usage: qlaunch [options] <DIRECTORY>|<QLAUNCH ITEM>" ) ,
                      TEXT ( "Options: -l lists folder content" ) ,
                      TEXT ( "Options: -o open qlaunch item" ) ) ;

    MessageBox ( g_hWnd , szHelp , TEXT ( "Quick Launch utility" ) , MB_ICONINFORMATION | MB_OK ) ;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Opens file/application identified using the specified quick launch item name.
VOID LaunchItem ( LPCTSTR lpszItemName )
{
    CConfigReader config ( lpszItemName ) ;
    SHELLEXECUTEINFO seiExecInfo ;
    if ( !config.Parse ( &seiExecInfo ) )
    {
        MessageBox ( g_hWnd , 
                     TEXT ( "Invalid quick launch item name passed as parameter" ) , 
                     TEXT ( "Invalid Parameter" ) , 
                     MB_ICONERROR | MB_OK ) ;
        return ;
    }

    if ( !ShellExecuteEx ( &seiExecInfo ) )
    {
        MessageBox ( g_hWnd , 
                     TEXT ( "Could not launch the specified item" ) , 
                     TEXT ( "ShellExecute error" ) , 
                     MB_ICONERROR | MB_OK ) ;
        return ;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Parses command line options and invokes required functionality or reports an error to user.
// Returns TRUE if application should stay alive, FALSE if job is done and we need to exit.
BOOL ParseCommandLine ( LPCTSTR lpCmdLine )
{
    // First we need to parse command line options and perform validation.
    LPWSTR *szArglist;
    int nArgs;
    HRESULT hr ;

    szArglist = CommandLineToArgvW ( lpCmdLine , &nArgs ) ;

    if ( 2 != nArgs ) // we always expect 2 arguments - option and parameter
    {
        Usage ( ) ;
        return FALSE ;
    }

    switch ( szArglist [ 0 ] [ 1 ] )
    {
    case L'l': // User wants to list directory content
        // Copy path of the directory to list to the global buffer
        hr = StringCchCopy ( g_szTargetDir , MAX_PATH , szArglist [ 1 ] ) ;
        if ( FAILED ( hr ) )
        {
            MessageBox ( g_hWnd , 
                         TEXT ( "Invalid directory name passed as parameter" ) , 
                         TEXT ( "Invalid Parameter" ) , 
                         MB_ICONERROR | MB_OK ) ;
            return FALSE ;
        }

        // And ask directory lister to perform it's work
        g_pLister = new CDirectoryLister ( g_hWnd ) ;
        if ( -1 == g_pLister->ShowMenu ( g_szTargetDir ) )
        {
            MessageBox ( g_hWnd , 
                         g_szTargetDir , 
                         TEXT ( "Could not list directory" ) , 
                         MB_OK | MB_ICONERROR ) ;
            return FALSE ;
        }

        return TRUE ; // Wait while user selects something from the popup menu
    case L'o': // User wants to launch a predefined application
        LaunchItem ( szArglist [ 1 ] ) ;

        return FALSE ;
    default:
        Usage ( ) ;
        return FALSE ;
    }
}