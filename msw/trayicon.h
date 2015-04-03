#ifndef _TRAYICON_H
#define _TRAYICON_H

#include <tchar.h>
#include <windows.h>

#define HELP_ABOUT _T("KV Command line dictionary tool.\n\nhttps://github.com/brookhong/kv")

#define THIS_CLASSNAME      _T("KV")
#define THIS_TITLE          _T("KV")

#define IDI_TRAY       100
#define WM_TRAYMSG     101
#define MENU_CONFIG    32
#define MENU_EXIT      33


void app_close_listener( HWND );
void    RegisterApplicationClass( HINSTANCE hInstance );

#endif
