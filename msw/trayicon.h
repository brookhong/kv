#ifndef _TRAYICON_H
#define _TRAYICON_H

#include <tchar.h>
#include <windows.h>

#define HELP_ABOUT _T("KV Command line dictionary tool.\n\nhttps://github.com/brookhong/kv")

#define THIS_CLASSNAME      _T("KV")
#define THIS_TITLE          _T("KV")


enum {
    ID_TRAYICON         = 1,

    APPWM_TRAYICON      = WM_APP,
    APPWM_NOP           = WM_APP + 1,

    //  Our commands
    ID_EXIT             = 2000,
    ID_ABOUT,
};

extern void (*app_close_listener)( HWND );
extern LRESULT (*WindowProc_fallback)( HWND, UINT, WPARAM, LPARAM );

void    RegisterApplicationClass( HINSTANCE hInstance );

#endif
