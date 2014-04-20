#include "trayicon.h"

static LRESULT CALLBACK WindowProc( HWND hWnd, UINT uMsg, WPARAM wParam,
        LPARAM lParam )
{
    static NOTIFYICONDATA nid;
    static HMENU hPopMenu;
    switch ( uMsg ) {
        // trayicon
        case WM_CREATE:
            {
                memset( &nid, 0, sizeof( nid ) );

                nid.cbSize              = sizeof( nid );
                nid.hWnd                = hWnd;
                nid.uID                 = IDI_TRAY;
                nid.uFlags              = NIF_ICON | NIF_MESSAGE | NIF_TIP;
                nid.uCallbackMessage    = WM_TRAYMSG;
                TCHAR    szIconFile[512];
                GetSystemDirectory( szIconFile, sizeof( szIconFile ) );
                if ( szIconFile[ lstrlen( szIconFile ) - 1 ] != '\\' )
                    lstrcat( szIconFile, _T("\\") );
                lstrcat( szIconFile, _T("shell32.dll") );
                ExtractIconEx( szIconFile, 17, NULL, &(nid.hIcon), 1 );
                lstrcpy( nid.szTip, HELP_ABOUT);
                Shell_NotifyIcon( NIM_ADD, &nid );

                hPopMenu = CreatePopupMenu();
                AppendMenu( hPopMenu, MF_STRING, MENU_CONFIG,  "&About..." );
                AppendMenu( hPopMenu, MF_STRING, MENU_EXIT,    "E&xit" );
                SetMenuDefaultItem( hPopMenu, MENU_CONFIG, FALSE );
            }
            break;
        case WM_TRAYMSG:
            {
                switch ( lParam )
                {
                    case WM_RBUTTONUP:
                        {
                            POINT pnt;
                            GetCursorPos( &pnt );
                            SetForegroundWindow( hWnd ); // needed to get keyboard focus
                            TrackPopupMenu( hPopMenu, TPM_LEFTALIGN, pnt.x, pnt.y, 0, hWnd, NULL );
                        }
                        break;
                    case WM_LBUTTONDBLCLK:
                        SendMessage( hWnd, WM_COMMAND, MENU_CONFIG, 0 );
                        return 0;
                }
            }
            break;
        case WM_COMMAND:
            {
                switch ( LOWORD( wParam ) )
                {
                    case MENU_CONFIG:
                        MessageBox( hWnd, HELP_ABOUT, THIS_TITLE,
                                MB_ICONINFORMATION | MB_OK );
                        return 0;
                    case MENU_EXIT:
                        PostMessage( hWnd, WM_CLOSE, 0, 0 );
                        return 0;
                    default:
                        // This happens when the right-click menu is canceled.
                        return 0;
                }
            }
            break;
        case WM_CLOSE:
            if ( app_close_listener )
                (*app_close_listener)( hWnd );
            PostQuitMessage( 0 );
            return DefWindowProc( hWnd, uMsg, wParam, lParam );
        default:
            return DefWindowProc( hWnd, uMsg, wParam, lParam );
    }
    return 0;
}

void RegisterApplicationClass( HINSTANCE hInstance )
{
    WNDCLASSEX wclx;
    memset( &wclx, 0, sizeof( wclx ) );

    wclx.cbSize         = sizeof( wclx );
    wclx.style          = 0;
    wclx.lpfnWndProc    = &WindowProc;
    wclx.cbClsExtra     = 0;
    wclx.cbWndExtra     = 0;
    wclx.hInstance      = hInstance;
    wclx.hCursor        = LoadCursor( NULL, IDC_ARROW );
    wclx.hbrBackground  = (HBRUSH)( COLOR_BTNFACE + 1 );    //  COLOR_* + 1 is
    //  special magic.
    wclx.lpszMenuName   = NULL;
    wclx.lpszClassName  = THIS_CLASSNAME;

    RegisterClassEx( &wclx );
}
