#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include "defproc.h"

#define TARGET_PLATFORM TEXT("Win32")
#define TARGET_CPU TEXT("IA-32(x86)")
#define COMPILER_NAME TEXT("Borland C++ 5.5.1 for Win32 w/ BCC Developer")

#define WND_CLASS_NAME TEXT("File_Separation_Main")
#define NUMOFSTRINGTABLE 29 // String Table に含まれる文字列の数
#define APP_SETFOCUS WM_APP // WM_APP から 0xBFFF までは自作メッセージとして使える

struct INPUTBOX{ // 入力受付用
    TCHAR path[MAX_PATH];
    TCHAR name[MAX_PATH];
    INT sepunit;
};

HWND hwnd, hbtn_sel, hbtn_ok, hbtn_abort, hbtn_clr, hedi_path, hedi_name, hedi_sepunit, hedi_out, hwnd_temp, hwnd_focused;
HINSTANCE hInst;
HMENU hmenu;
HACCEL hAccel;
DWORD dThreadID;
HANDLE hThread;
HDC hdc, hMemDC;
HFONT hMesFont, hFbtn, hFedi, hFnote; // 作成するフォント
LOGFONT rLogfont; // 作成するフォントの構造体
HBRUSH hBshSys, hBrush; // 取得するブラシ
HPEN hPenSys, hPen; // 取得するペン
HBITMAP hBitmap;
PAINTSTRUCT ps;
RECT rect;
INT r=0, g=255, b=255, scrx=0, scry=0, editlen=0, btnsize[2];
bool aborted=false, curdir=false, working=false;
TCHAR tcmes[NUMOFSTRINGTABLE][1024], tctemp[1024];
INPUTBOX InputBox; // 入力受付
WNDPROC wpedipath_old, wpediname_old, wpedisepunit_old; // 元の Window Procedure のアドレス格納用変数

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditPathWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSepWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditNameWindowProc(HWND, UINT, WPARAM, LPARAM);
INT CALLBACK SelDirProc(HWND, UINT, LPARAM, LPARAM);
DWORD WINAPI SeparateFiles(LPVOID);
void Paint();
void ResizeMoveControls();
void SelDir();
void OutputToEditbox(HWND hWnd, LPCTSTR arg){ // エディットボックスの末尾に文字列を追加
    INT EditLen = (INT)SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
    SendMessage(hWnd, EM_SETSEL, EditLen, EditLen);
    SendMessage(hWnd, EM_REPLACESEL, 0, (WPARAM)arg);
    return;
}

extern "C" int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow){
    hInst = hInstance;

    WNDCLASSEX wcl;
    wcl.cbSize = sizeof(WNDCLASSEX);
    wcl.hInstance = hInstance;
    wcl.lpszClassName = WND_CLASS_NAME;
    wcl.lpfnWndProc = WindowProc;
    wcl.style = 0;
    wcl.hIcon = LoadIcon(hInstance, TEXT("Res_Icon")); // アイコンの読み込み
    wcl.hIconSm = LoadIcon(hInstance, TEXT("Res_Icon"));
    wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcl.lpszMenuName = 0;
    wcl.cbClsExtra = 0;
    wcl.cbWndExtra = 0;
    wcl.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    if(!RegisterClassEx(&wcl)) return FALSE;

    // 画面サイズを取得して、ウィンドウサイズを決定
    SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
    if(rect.bottom*5/3 > rect.right){
        scrx = rect.right*3/4;
        scry = rect.right*9/20;
    }else{
        scrx = rect.bottom*10/9;
        scry = rect.bottom*2/3;
    }
    if(scrx<800 || scry<480) {scrx=800; scry=480;}

    if(GetUserDefaultUILanguage() == 0x0411){ // UIが日本語環境なら日本語リソースを読み込む
        for(int i=0; i<NUMOFSTRINGTABLE; i++) LoadString(hInstance, i+1000, tcmes[i], sizeof(tcmes[0])/sizeof(tcmes[0][0]));
        hmenu = LoadMenu(hInstance, TEXT("Res_JapaneseMenu"));
    } else{ // それ以外なら英語リソースを読み込む
        for(int i=0; i<NUMOFSTRINGTABLE; i++) LoadString(hInstance, i+1100, tcmes[i], sizeof(tcmes[0])/sizeof(tcmes[0][0]));
        hmenu = LoadMenu(hInstance, TEXT("Res_EnglishMenu"));
    }
    
    hwnd = CreateWindowEx(
        0,
        WND_CLASS_NAME,
        tcmes[0],
        WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        scrx,
        scry,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    hbtn_sel = CreateWindowEx( // フォルダ選択ボタン
        0,
        TEXT("BUTTON"),
        TEXT("..."),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 
        0,
        0,
        0,
        hwnd,
        (HMENU)0,
        hInstance,
        NULL);

    hbtn_ok = CreateWindowEx( // OKボタン
        0,
        TEXT("BUTTON"),
        tcmes[9],
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 
        0,
        0,
        0,
        hwnd,
        (HMENU)1,
        hInstance,
        NULL);

    hbtn_abort = CreateWindowEx( // 中断ボタン
        0,
        TEXT("BUTTON"),
        tcmes[10],
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        0,
        0,
        0,
        0,
        hwnd,
        (HMENU)2,
        hInstance,
        NULL);

    hbtn_clr = CreateWindowEx( // 履歴消去ボタン
        0,
        TEXT("BUTTON"),
        tcmes[11],
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0,
        0,
        0,
        0,
        hwnd,
        (HMENU)3,
        hInstance,
        NULL);
    
    if(!hwnd) return FALSE;
     
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);
    hAccel = LoadAccelerators(hInstance, TEXT("Res_Accel"));
    
    MSG msg;
    SetTimer(hwnd, 1, 16, NULL);
    
    while(GetMessage(&msg, NULL, 0, 0)){ // メッセージが WM_QUIT(=0) でない限りループ
        if(!TranslateAccelerator(hwnd, hAccel, &msg)){
            TranslateMessage(&msg);
            DispatchMessage(&msg); 
        }
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch(uMsg){
        case WM_CREATE: // ウィンドウ生成時
            SetMenu(hWnd, hmenu);
            if(GetUserDefaultUILanguage() == 0x0411) CheckMenuRadioItem(hmenu, 2080, 2081, 2080, MF_BYCOMMAND);
            else CheckMenuRadioItem(hmenu, 2080, 2081, 2081, MF_BYCOMMAND);
            hMemDC = CreateCompatibleDC(NULL);
            hBshSys = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            hPenSys = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));

            hedi_path = CreateWindowEx( // 操作対象パス入力ボックス
                0,
                TEXT("EDIT"),
                TEXT(""),
                WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_AUTOHSCROLL,
                0, 
                0,
                0,
                0,
                hWnd,
                (HMENU)50,
                ((LPCREATESTRUCT)(lParam))->hInstance,
                NULL);
            SendMessage(hedi_path, EM_SETLIMITTEXT, (WPARAM)MAX_PATH, 0);
            wpedipath_old = (WNDPROC)SetWindowLongPtr(hedi_path, GWLP_WNDPROC, (LONG_PTR)EditPathWindowProc); // Window Procedureのすり替え
            SetFocus(hedi_path); hwnd_focused = hedi_path;
                
            hedi_name = CreateWindowEx( // ファイル名入力ボックス
                0,
                TEXT("EDIT"),
                TEXT("*.*"),
                WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_AUTOHSCROLL,
                0, 
                0,
                0,
                0,
                hWnd,
                (HMENU)51,
                ((LPCREATESTRUCT)(lParam))->hInstance,
                NULL);
            wpediname_old = (WNDPROC)SetWindowLongPtr(hedi_name, GWLP_WNDPROC, (LONG_PTR)EditNameWindowProc); // Window Procedureのすり替え
                
            hedi_sepunit = CreateWindowEx( // 分割単位入力ボックス
                0,
                TEXT("EDIT"),
                TEXT("20"),
                WS_CHILD | WS_VISIBLE | ES_LEFT | WS_BORDER | ES_AUTOHSCROLL | ES_NUMBER,
                0, 
                0,
                0,
                0,
                hWnd,
                (HMENU)52,
                ((LPCREATESTRUCT)(lParam))->hInstance,
                NULL);
            wpedisepunit_old = (WNDPROC)SetWindowLongPtr(hedi_sepunit, GWLP_WNDPROC, (LONG_PTR)EditSepWindowProc); // Window Procedureのすり替え
        
            hedi_out = CreateWindowEx( // 出力表示ボックス
                0,
                TEXT("EDIT"),
                TEXT(""),
                WS_CHILD | WS_VISIBLE | ES_READONLY | ES_LEFT | WS_BORDER | ES_MULTILINE | ES_AUTOVSCROLL | WS_VSCROLL,
                0,
                0,
                0,
                0,
                hWnd,
                (HMENU)60,
                ((LPCREATESTRUCT)(lParam))->hInstance,
                NULL);
            break;
      
        case WM_CLOSE: // 終了時
            KillTimer(hWnd, 1);
            DestroyWindow(hwnd);
            break;
      
        case WM_TIMER:
            if( (hwnd_temp=GetFocus()) && (hwnd_temp==hedi_path || hwnd_temp==hedi_sepunit || hwnd_temp==hedi_name || hwnd_temp==hedi_out)) hwnd_focused = hwnd_temp;
            
            if(b<=0 && g<255) g++; //
            if(g>=255 && r>0) r--; //
            if(r<=0 && b<255) b++; //
            if(b>=255 && g>0) g--; //
            if(g<=0 && r<255) r++; //
            if(r>=255 && b>0) b--; //
                                   //
            if(r>255) r=255;       //
            if(r<0) r=0;           //
            if(g>255) g=255;       //
            if(g<0) g=0;           //
            if(b>255) b=255;       //
            if(b<0) b=0;           //

            DeleteObject(hBrush);
            DeleteObject(hPen);
            hBrush = CreateSolidBrush(RGB(r, g, b)); // ブラシを作成(塗りつぶし用)
            hPen = CreatePen(PS_SOLID, 1, RGB(r, g, b)); // ペンを作成(輪郭用)
            InvalidateRect(hWnd, NULL, FALSE);
            break;
            
        case WM_SIZE: // ウィンドウサイズ変更時
            ResizeMoveControls();
            break;

        case WM_PAINT:
            Paint();
            break;

        case WM_ACTIVATE:
            // メインウィンドウがアクティブ化されたとき、以前フォーカスが当たっていたエディットにフォーカスを戻す
            if(LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) SetFocus(hwnd_focused);
            break;
        
        case WM_COMMAND:
            switch(LOWORD(wParam)){
                case 0: // ディレクトリ選択
                    SelDir();
                    break;

                case 1: // OKボタン
                    if(working) break;
                    working = true;
                    EnableWindow(hbtn_ok, FALSE);
                    EnableWindow(hedi_path, FALSE);
                    EnableWindow(hedi_name, FALSE);
                    EnableWindow(hedi_sepunit, FALSE);
                    EnableWindow(hbtn_abort, TRUE);
                    EnableMenuItem(hmenu, 2, MF_BYPOSITION | MF_GRAYED); // 「Language」メニューをグレーアウト
                    DrawMenuBar(hwnd);

                    SendMessage(hedi_path, WM_GETTEXT, MAX_PATH, (LPARAM)InputBox.path);
                    SendMessage(hedi_name, WM_GETTEXT, MAX_PATH, (LPARAM)InputBox.name);
                    SendMessage(hedi_sepunit, WM_GETTEXT, 10, (LPARAM)tctemp);
                    InputBox.sepunit = _ttoi(tctemp);
                    curdir = false;
                    if(SendMessage(hedi_path, WM_GETTEXTLENGTH, 0, 0)==0) curdir = true;
                    
                    hThread = CreateThread(NULL, 0, SeparateFiles, 0, 0, &dThreadID);
                    SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
                    break;

                case 2: // 中断
                    aborted = true;
                    break;

                case 3: // 履歴消去
                    editlen = SendMessage(hedi_out, WM_GETTEXTLENGTH, 0, 0);
                    SendMessage(hedi_out, EM_SETSEL, 0, editlen);
                    SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT(""));
                    break;
                
                case 2009: // 終了
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    break;

                case 2020: // 貼り付け
                    SendMessage(hwnd_focused, WM_PASTE, 0, 0);
                    break;

                case 2021: // 全選択
                    editlen = (INT)SendMessage(hwnd_focused, WM_GETTEXTLENGTH, 0, 0);
                    SendMessage(hwnd_focused, EM_SETSEL, 0, editlen);
                    break;

                case 2080: // 日本語に変更
                    DestroyMenu(hmenu);
                    hmenu = LoadMenu(hInst, TEXT("Res_JapaneseMenu"));
                    SetMenu(hWnd, hmenu);
                    CheckMenuRadioItem(hmenu, 2080, 2081, 2080, MF_BYCOMMAND);

                    for(int i=0; i<NUMOFSTRINGTABLE; i++) LoadString(hInst, i+1000, tcmes[i], sizeof(tcmes[0])/sizeof(tcmes[0][0]));
                    SetWindowText(hbtn_ok, tcmes[9]);
                    SetWindowText(hbtn_abort, tcmes[10]);
                    SetWindowText(hbtn_clr, tcmes[11]);
                    SetWindowText(hwnd, tcmes[0]);
                    break;

                case 2081: // 英語に変更
                    DestroyMenu(hmenu);
                    hmenu = LoadMenu(hInst, TEXT("Res_EnglishMenu"));
                    SetMenu(hWnd, hmenu);
                    CheckMenuRadioItem(hmenu, 2080, 2081, 2081, MF_BYCOMMAND);

                    for(int i=0; i<NUMOFSTRINGTABLE; i++) LoadString(hInst, i+1100, tcmes[i], sizeof(tcmes[0])/sizeof(tcmes[0][0]));
                    SetWindowText(hbtn_ok, tcmes[9]);
                    SetWindowText(hbtn_abort, tcmes[10]);
                    SetWindowText(hbtn_clr, tcmes[11]);
                    SetWindowText(hwnd, tcmes[0]);
                    break;

                case 2101: // 使い方
                    MessageBox(hWnd, tcmes[8], tcmes[7], MB_OK | MB_ICONINFORMATION);
                    break;

                case 2109: // このプログラムについて
                    wsprintf(tctemp, TEXT("%s")
                        TEXT("%s") COMPILER_NAME TEXT("\n")
                        TEXT("%s") TARGET_PLATFORM TEXT(" Application\n")
                        TEXT("%s") TARGET_CPU TEXT("\n")
                        TEXT("%s") TEXT(__DATE__) TEXT(" ") TEXT(__TIME__)
                        TEXT("\n\nCopyright (C) 2019-2020 watamario15 All rights reserved."),
                        tcmes[2], tcmes[3], tcmes[4], tcmes[5], tcmes[6]);
                    MessageBox(hWnd, tctemp, tcmes[1], MB_OK | MB_ICONINFORMATION);
                    break;
            }
            if(LOWORD(wParam)<50) SetFocus(hwnd_focused); // エディットコントロール以外ならエディットコントロールにフォーカスを戻す
            else return DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;

        case APP_SETFOCUS:
            SetFocus(hwnd_focused);
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            break;

        default:
            return myDefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// エディットコントロール用のサブクラス化 Window Procedure
LRESULT CALLBACK EditPathWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch(uMsg){
        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wpedipath_old);
            break;

        // Enterで次のエディットへ, 実行対象でなければdefaultに流す(間に別のメッセージ入れちゃだめ!!)
        case WM_CHAR:
            switch((CHAR)wParam){
                case VK_RETURN:
                    SetFocus(hedi_name);
                    return 0;
            }
        default:
            return CallWindowProc(wpedipath_old, hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK EditNameWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch(uMsg){
        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wpediname_old);
            break;

        // Enterで次のエディットへ, 実行対象でなければdefaultに流す(間に別のメッセージ入れちゃだめ!!)
        case WM_CHAR:
            switch((CHAR)wParam){
                case VK_RETURN:
                    SetFocus(hedi_sepunit);
                    return 0;
            }
        default:
            return CallWindowProc(wpediname_old, hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

LRESULT CALLBACK EditSepWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch(uMsg){
        case WM_DESTROY:
            SetWindowLongPtr(hWnd, GWLP_WNDPROC, (LONG_PTR)wpedisepunit_old);
            break;

        // Enterで処理開始, 実行対象でなければdefaultに流す(間に別のメッセージ入れちゃだめ!!)
        case WM_CHAR:
            switch((CHAR)wParam){
                case VK_RETURN:
                    if(working) break;
                    working = true;
                    EnableWindow(hbtn_ok, FALSE);
                    EnableWindow(hedi_path, FALSE);
                    EnableWindow(hedi_name, FALSE);
                    EnableWindow(hedi_sepunit, FALSE);
                    EnableWindow(hbtn_abort, TRUE);
                    EnableMenuItem(hmenu, 2, MF_BYPOSITION | MF_GRAYED); // 「オプション」メニューをグレーアウト
                    DrawMenuBar(hwnd);

                    SendMessage(hedi_path, WM_GETTEXT, MAX_PATH, (LPARAM)InputBox.path);
                    SendMessage(hedi_name, WM_GETTEXT, MAX_PATH, (LPARAM)InputBox.name);
                    SendMessage(hedi_sepunit, WM_GETTEXT, 10, (LPARAM)tctemp);
                    InputBox.sepunit = _ttoi(tctemp);
                    curdir = false;
                    if(SendMessage(hedi_path, WM_GETTEXTLENGTH, 0, 0)==0) curdir=true;

                    hThread = CreateThread(NULL, 0, SeparateFiles, 0, 0, &dThreadID);
                    SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
                    return 0;
            }
        default:
            return CallWindowProc(wpedisepunit_old, hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT CALLBACK SelDirProc(HWND hWnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2){
    switch (uMsg){
        case BFFM_VALIDATEFAILED:
            MessageBox(hWnd, tcmes[12], tcmes[13], MB_OK | MB_ICONEXCLAMATION);
            return 1;
    }
    return 0;
}

void SelDir(){
    INT nItem = -1;
    TCHAR szName[MAX_PATH];
    BOOL bDir = FALSE;
    BROWSEINFO bi;
    ITEMIDLIST *lpid;
    HRESULT hr;
    LPMALLOC pMalloc = NULL; // IMallocへのポインタ

    ZeroMemory(&bi, sizeof(BROWSEINFO));
    bi.hwndOwner = hwnd;
    bi.lpfn = SelDirProc;
    bi.ulFlags = BIF_EDITBOX | BIF_VALIDATE | BIF_NEWDIALOGSTYLE;
    bi.lpszTitle = tcmes[14];
    lpid = SHBrowseForFolder(&bi);

    if(lpid){
        hr = SHGetMalloc(&pMalloc);
        if(hr == E_FAIL){
            MessageBox(hwnd, tcmes[15], tcmes[13], MB_OK | MB_ICONERROR);
            return;
        }
        SHGetPathFromIDList(lpid, szName);
        if(_tcscmp(szName, TEXT("\\"))==0) _tcscpy(szName, TEXT(""));
        SetWindowText(hedi_path, szName);
        pMalloc->Free(lpid);
        pMalloc->Release();
    }
    SendMessage(hwnd, APP_SETFOCUS, 0, 0);
    return;
}

void ResizeMoveControls(){
    GetClientRect(hwnd, &rect);
    scrx = rect.right; scry = rect.bottom;
    
    // メインウィンドウ用のフォントを作成
    if(scrx/24 < scry/12) rLogfont.lfHeight = scrx/24;
    else rLogfont.lfHeight = scry/12;
    rLogfont.lfWidth = 0;
    rLogfont.lfEscapement = 0;
    rLogfont.lfOrientation = 0;
    rLogfont.lfWeight = FW_EXTRABOLD;
    rLogfont.lfItalic = TRUE;
    rLogfont.lfUnderline = TRUE;
    rLogfont.lfStrikeOut = FALSE;
    rLogfont.lfCharSet = SHIFTJIS_CHARSET;
    rLogfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    rLogfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    rLogfont.lfQuality = DEFAULT_QUALITY;
    rLogfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    _tcscpy(rLogfont.lfFaceName, TEXT("MS PGothic"));
    DeleteObject(hMesFont);
    hMesFont = CreateFontIndirect(&rLogfont); // フォントを作成

    // ボタン用のフォントを作成
    if(24*scrx/700 < 24*scry/400) rLogfont.lfHeight = 24*scrx/700;
    else  rLogfont.lfHeight = 24*scry/400;
    rLogfont.lfWidth = 0;
    rLogfont.lfEscapement = 0;
    rLogfont.lfOrientation = 0;
    rLogfont.lfWeight = FW_NORMAL;
    rLogfont.lfItalic = FALSE;
    rLogfont.lfUnderline = FALSE;
    rLogfont.lfStrikeOut = FALSE;
    rLogfont.lfCharSet = SHIFTJIS_CHARSET;
    rLogfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    rLogfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    rLogfont.lfQuality = DEFAULT_QUALITY;
    rLogfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    _tcscpy(rLogfont.lfFaceName, TEXT("MS PGothic"));
    DeleteObject(hFbtn);
    hFbtn = CreateFontIndirect(&rLogfont);
    
    // 注用のフォントを作成
    if(12*scrx/700 < 12*scry/400) rLogfont.lfHeight = 12*scrx/700;
    else rLogfont.lfHeight = 12*scry/400;
    DeleteObject(hFnote);
    hFnote = CreateFontIndirect(&rLogfont);

    // エディットコントロール用のフォントを作成
    if(16*scrx/700 < 16*scry/400) rLogfont.lfHeight = 16*scrx/700;
    else rLogfont.lfHeight = 16*scry/400;
    if(rLogfont.lfHeight<12) rLogfont.lfHeight = 12;
    rLogfont.lfWidth = 0;
    rLogfont.lfEscapement = 0;
    rLogfont.lfOrientation = 0;
    rLogfont.lfWeight = FW_NORMAL;
    rLogfont.lfItalic = FALSE;
    rLogfont.lfUnderline = FALSE;
    rLogfont.lfStrikeOut = FALSE;
    rLogfont.lfCharSet = SHIFTJIS_CHARSET;
    rLogfont.lfOutPrecision = OUT_DEFAULT_PRECIS;
    rLogfont.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    rLogfont.lfQuality = DEFAULT_QUALITY;
    rLogfont.lfPitchAndFamily = VARIABLE_PITCH | FF_SWISS;
    _tcscpy(rLogfont.lfFaceName, TEXT("MS Gothic"));
    DeleteObject(hFedi);
    hFedi = CreateFontIndirect(&rLogfont);
    
    // 各コントロールにフォントを適用
    SendMessage(hbtn_sel, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hbtn_ok, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hbtn_abort, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hbtn_clr, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hedi_name, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hedi_sepunit, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hedi_path, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hedi_out, WM_SETFONT, (WPARAM)hFedi, MAKELPARAM(FALSE, 0));

    // 移動とサイズ変更
    btnsize[0] = 64*scrx/700; btnsize[1] = 32*scry/400;
    MoveWindow(hedi_path, btnsize[0], 0, scrx-btnsize[0]*2, btnsize[1], TRUE);
    MoveWindow(hbtn_sel, scrx-btnsize[0], 0, btnsize[0], btnsize[1], TRUE);
    MoveWindow(hedi_name, btnsize[0], btnsize[1], btnsize[0]*2, btnsize[1], TRUE);
    MoveWindow(hedi_sepunit, btnsize[0]*4, btnsize[1], btnsize[0]*2, btnsize[1], TRUE);
    MoveWindow(hbtn_ok, btnsize[0]*6, btnsize[1], btnsize[0], btnsize[1], TRUE);
    MoveWindow(hbtn_abort, btnsize[0]*7, btnsize[1], btnsize[0], btnsize[1], TRUE);
    MoveWindow(hbtn_clr, btnsize[0]*8, btnsize[1], btnsize[0]*5/2, btnsize[1], TRUE);
    MoveWindow(hedi_out, scrx/20, scry*3/10, scrx*9/10, scry*13/20, TRUE);
    
    DeleteObject(hBitmap);
    hBitmap = CreateCompatibleBitmap(hdc=GetDC(hwnd), rect.right, rect.bottom);
    ReleaseDC(hwnd, hdc);
    SelectObject(hMemDC, hBitmap);
    InvalidateRect(hwnd, NULL, FALSE);
    return;
}

void Paint(){
    GetClientRect(hwnd, &rect);
    SelectObject(hMemDC, hPen); // デバイスコンテキストとペンをつなぐ
    SelectObject(hMemDC, hBrush); // デバイスコンテキストとブラシをつなぐ
    Rectangle(hMemDC, rect.left, rect.top, rect.right, rect.bottom); // 領域いっぱいに四角形を描く
    
    SelectObject(hMemDC, hPenSys);
    SelectObject(hMemDC, hBshSys);
    Rectangle(hMemDC, 0, 0, btnsize[0], btnsize[1]*2);
    Rectangle(hMemDC, btnsize[0]*3, btnsize[1], btnsize[0]*4, btnsize[1]*2);
    
    SetTextColor(hMemDC, RGB(0, 0, 0));
    SetBkMode(hMemDC, TRANSPARENT);
    SelectObject(hMemDC, hFnote); // デバイスコンテキストにフォントを設定
    rect.right=btnsize[0]; rect.bottom=btnsize[1];
    DrawText(hMemDC, tcmes[16], -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    rect.top=btnsize[1]; rect.bottom=btnsize[1]*2;
    DrawText(hMemDC, tcmes[17], -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    rect.top=btnsize[1]; rect.bottom=btnsize[1]*2; rect.left=btnsize[0]*3; rect.right=btnsize[0]*4;
    DrawText(hMemDC, tcmes[18], -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SetBkMode(hMemDC, OPAQUE);
    SetBkColor(hMemDC, RGB(255, 255, 0));
    SetTextColor(hMemDC, RGB(0, 0, 255));
    SelectObject(hMemDC, hMesFont);
    rect.left=0; rect.right=scrx; rect.top=btnsize[1]*2; rect.bottom=scry*3/10;
    if(working) DrawText(hMemDC, tcmes[19], -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    else DrawText(hMemDC, tcmes[20], -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    rect.left=0; rect.right=scrx; rect.top=0; rect.bottom=scry;
    
    hdc = BeginPaint(hwnd, &ps);
    BitBlt(hdc, 0, 0, rect.right, rect.bottom, hMemDC, 0, 0, SRCCOPY);
    EndPaint(hwnd, &ps);
    return;
}

DWORD WINAPI SeparateFiles(LPVOID lpParameter){
    INT dirnum=1, i=0;
    TCHAR tcsf[MAX_PATH]=TEXT(""), tcdest[MAX_PATH], tcresult[1024];
    HANDLE hFFile;
    WIN32_FIND_DATA file;

    if(InputBox.sepunit<=0){ // 分割単位値不正
        EnableWindow(hbtn_ok, TRUE);
        EnableWindow(hedi_path, TRUE);
        EnableWindow(hedi_name, TRUE);
        EnableWindow(hedi_sepunit, TRUE);
        EnableWindow(hbtn_abort, FALSE);
        EnableMenuItem(hmenu, 2, MF_BYPOSITION | MF_ENABLED);
        DrawMenuBar(hwnd);
        OutputToEditbox(hedi_out, tcmes[21]);
        SetWindowText(hwnd, tcmes[0]);
        SendMessage(hwnd, APP_SETFOCUS, 0, 0);
        working=false;
        return 0;
    }
    
    if(!curdir) wsprintf(tcsf, TEXT("%s\\%s"), InputBox.path, InputBox.name);
    else wsprintf(tcsf, TEXT("%s"), InputBox.name);
    hFFile = FindFirstFile(tcsf, &file);
    if(hFFile==INVALID_HANDLE_VALUE){ // 該当ファイルが存在しない場合
        EnableWindow(hbtn_ok, TRUE);
        EnableWindow(hedi_path, TRUE);
        EnableWindow(hedi_name, TRUE);
        EnableWindow(hedi_sepunit, TRUE);
        EnableWindow(hbtn_abort, FALSE);
        EnableMenuItem(hmenu, 2, MF_BYPOSITION | MF_ENABLED);
        DrawMenuBar(hwnd);
        OutputToEditbox(hedi_out, tcmes[22]);
        SetWindowText(hwnd, tcmes[0]);
        SendMessage(hwnd, APP_SETFOCUS, 0, 0);
        working=false;
        return 0;
    }
    
    if(!curdir) wsprintf(tcsf, TEXT("%s\\1"), InputBox.path);
    else wsprintf(tcsf, TEXT("1"), InputBox.path);
    CreateDirectory(tcsf, NULL);
    if(!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        _tcscpy(tcresult, tcmes[23]);
        SetWindowText(hwnd, tcresult);
        OutputToEditbox(hedi_out, tcresult);
        SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT("\r\n"));
        if(curdir){
            _tcscpy(tcsf, file.cFileName);
            wsprintf(tcdest, TEXT(".\\1\\%s"), file.cFileName);
        }else{
            wsprintf(tcsf, TEXT("%s\\%s"), InputBox.path, file.cFileName);
            wsprintf(tcdest, TEXT("%s\\1\\%s"), InputBox.path, file.cFileName);
        }
        if(!MoveFile(tcsf, tcdest)){
            wsprintf(tcresult, tcmes[24], tcsf, tcdest);
            OutputToEditbox(hedi_out, tcresult);
        }
        i++;
    }
    
    while(!aborted){
        if(FindNextFile(hFFile, &file)){
            if(!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
                if(i==0){
                    _tcscpy(tcresult, tcmes[23]);
                    SetWindowText(hwnd, tcresult);
                    OutputToEditbox(hedi_out, tcresult);
                    SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT("\r\n"));
                }
                if(i%InputBox.sepunit==0 && i!=0){
                    dirnum++;
                    wsprintf(tcsf, TEXT("%s\\%d"), InputBox.path, dirnum);
                    CreateDirectory(tcsf, NULL);
                    wsprintf(tcresult, tcmes[25], dirnum);
                    SetWindowText(hwnd,tcresult);
                    OutputToEditbox(hedi_out, tcresult);
                    SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT("\r\n"));
                }
                
                _tcscpy(tcsf, InputBox.path);
                if(curdir){
                    _tcscpy(tcsf, file.cFileName);
                    wsprintf(tcdest, TEXT(".\\%d\\%s"), dirnum, file.cFileName);
                }else{
                    wsprintf(tcsf, TEXT("%s\\%s"), InputBox.path, file.cFileName);
                    wsprintf(tcdest, TEXT("%s\\%d\\%s"), InputBox.path, dirnum, file.cFileName);
                }
                if(!MoveFile(tcsf, tcdest)){
                    wsprintf(tcresult, tcmes[24], tcsf, tcdest);
                    OutputToEditbox(hedi_out, tcresult);
                }
                i++;
            }
        }else{
            if(GetLastError()==ERROR_NO_MORE_FILES) _tcscpy(tcresult, tcmes[26]);
            else _tcscpy(tcresult, tcmes[27]);
            break;
        }
    }
    
    FindClose(hFFile);
    SetWindowText(hwnd, tcmes[0]);

    if(aborted){
        aborted = false;
        _tcscpy(tcresult, tcmes[28]);
    }

    OutputToEditbox(hedi_out, tcresult);
    
    EnableWindow(hbtn_ok, TRUE);
    EnableWindow(hedi_path, TRUE);
    EnableWindow(hedi_name, TRUE);
    EnableWindow(hedi_sepunit, TRUE);
    EnableWindow(hbtn_abort, FALSE);
    EnableMenuItem(hmenu, 2, MF_BYPOSITION | MF_ENABLED);
    DrawMenuBar(hwnd);
    InvalidateRect(hwnd, NULL, FALSE);
    SendMessage(hwnd, APP_SETFOCUS, 0, 0);
    working = false;
    return 0;
}