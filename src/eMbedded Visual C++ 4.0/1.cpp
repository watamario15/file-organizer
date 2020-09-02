#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shlobj.h>

#ifdef ARMV4
#define TARGET_CPU TEXT("ARMv4")
#endif
#ifdef ARMV4I
#define TARGET_CPU TEXT("ARMv4I")
#endif
#ifdef ARMV4T
#define TARGET_CPU TEXT("ARMv4T")
#endif
#ifdef EMULATOR // コンパイルオプションで手動定義
#define TARGET_CPU TEXT("Emulator")
#endif
#ifdef MIPSII // MIPSII, MIPS16, MIPSII_FP の並び順は変更禁止!!
#define TARGET_CPU TEXT("MIPSII")
#endif
#ifdef MIPS16
#define TARGET_CPU TEXT("MIPS16")
#endif
#ifdef MIPSII_FP
#define TARGET_CPU TEXT("MIPSII_FP")
#endif
#ifdef MIPSIV
#define TARGET_CPU TEXT("MIPSIV")
#endif
#ifdef MIPSIV_FP
#define TARGET_CPU TEXT("MIPSIV_FP")
#endif
#ifdef SH3
#define TARGET_CPU TEXT("SH3")
#endif
#ifdef SH4
#define TARGET_CPU TEXT("SH4")
#endif
#ifdef IA32 // コンパイルオプションで手動定義
#define TARGET_CPU TEXT("IA-32(x86)")
#endif

#define WND_CLASS_NAME TEXT("My_Window")
#define TIMER_AWAIT 250
#define APP_SETFOCUS WM_APP // WM_APP から 0xBFFF までは自作メッセージとして使える

// Dynamic Link 用関数のポインタ格納用の型を準備
typedef BOOL (*DLL_SHGetPathFromIDList)(LPCITEMIDLIST, WCHAR *);
typedef HRESULT (*DLL_SHGetMalloc)(IMalloc **);
typedef LPITEMIDLIST (*DLL_SHBrowseForFolder)(BROWSEINFO *);
struct INPUTBOX{ // 入力受付用
    TCHAR path[MAX_PATH];
    TCHAR name[MAX_PATH];
    INT sepunit;
};

HWND hwnd, hbtn_sel, hbtn_ok, hbtn_abort, hbtn_clr, hedi_path, hedi_name, hedi_sepunit, hedi_out, hCmdBar, hwnd_temp, hwnd_focused;
HINSTANCE hInst; // Instance Handle のバックアップ
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
INT r=0, g=255, b=255, scrx=0, scry=0, editlen=0, btnsize[2], CmdBar_Height;
bool aborted=false, TimerRestartable=true, withdll=true, working=false;
TCHAR tcmes[2][4096], tctemp[1024];
INPUTBOX InputBox; // 入力受付
WNDPROC wpedipath_old, wpediname_old, wpedisepunit_old;
DLL_SHGetPathFromIDList dll_SHGetPathFromIDList;
DLL_SHGetMalloc dll_SHGetMalloc;
DLL_SHBrowseForFolder dll_SHBrowseForFolder;

LRESULT CALLBACK WindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditPathWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditSepWindowProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditNameWindowProc(HWND, UINT, WPARAM, LPARAM);
INT CALLBACK SelDirProc(HWND, UINT, LPARAM, LPARAM);
DWORD WINAPI SeparateFiles(LPVOID);
void Paint();
void ResizeReplaceObjects();
void SelDir();
void OutputToEditbox(HWND hWnd, LPCTSTR arg){ // エディットボックスの末尾に文字列を追加
    INT EditLen = (INT)SendMessage(hWnd, WM_GETTEXTLENGTH, 0, 0);
    SendMessage(hWnd, EM_SETSEL, EditLen, EditLen);
    SendMessage(hWnd, EM_REPLACESEL, 0, (WPARAM)arg);
    return;
}

// 第3引数をLPSTR型からLPTSTR型に変更(CEの独自仕様)
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPTSTR lpCmdLine, int nCmdShow){
    hInst=hInstance;
    
	dll_SHGetPathFromIDList = (DLL_SHGetPathFromIDList)GetProcAddress(LoadLibrary(TEXT("ceshell.dll")), TEXT("SHGetPathFromIDList"));
	if(!dll_SHGetPathFromIDList) withdll=false;
	dll_SHGetMalloc = (DLL_SHGetMalloc)GetProcAddress(LoadLibrary(TEXT("ceshell.dll")), TEXT("SHGetMalloc"));
	if(!dll_SHGetMalloc) withdll=false;
	dll_SHBrowseForFolder = (DLL_SHBrowseForFolder)GetProcAddress(LoadLibrary(TEXT("ceshell.dll")), TEXT("SHBrowseForFolder"));
	if(!dll_SHBrowseForFolder) withdll=false;
    
    WNDCLASS wcl; // WNDCLASSEXは非対応
    wcl.hInstance = hInstance;
    wcl.lpszClassName = WND_CLASS_NAME;
    wcl.lpfnWndProc = (WNDPROC)WindowProc;
    wcl.style = NULL;
    wcl.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(100)); // load the icon
    wcl.hCursor = LoadCursor(NULL, IDC_ARROW);
    wcl.lpszMenuName = 0; // CEの場合、メニューリソースをここで読んでも無意味
    wcl.cbClsExtra = 0;
    wcl.cbWndExtra = 0;
    wcl.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
    if(!RegisterClass(&wcl)) return FALSE;

    // "how to use" statement
    _tcscpy(tcmes[0], TEXT("大量のファイルを指定した単位でフォルダに一括整理するシンプルなソフトウェアです。\n")
        TEXT("上の入力ボックスに整理したいパス(フルパス)、ファイル名(ワイルドカード可)、")
        TEXT("何フォルダ単位に振り分けるかを入力し、OKボタンまたはEnter(決定)で処理を開始します。\n")
        TEXT("「中断」ボタンで中断できますが、既に振り分けられたファイルを元に戻す機能はありません。基本的に辞書順に振り分けられるはずですが、")
        TEXT("そうならないこともあるようです(なぜそうなるのかは不明です)。必ず事前に付属のSandboxフォルダで動作を確認してください。")
    );

    // "about" statement
    _tcscpy(tcmes[1], TEXT("超大量ファイル整理 Ver. 1.02β\n\n")
        TEXT("製作者は、このソフトウェアの使用によって発生したバグ等による誤動作を含むいかなる損害に対しても補償致しません。自己責任でご利用ください。\n")
        TEXT("\n開発環境: Microsoft eMbedded Visual C++ 4.0\n")
        TEXT("プログラム種別: WinCE Application\n")
        TEXT("CPUアーキテクチャ: ") TARGET_CPU
        TEXT("\nビルド日時: ") TEXT(__DATE__) TEXT(" ") TEXT(__TIME__)
        TEXT("\n\n(C) 2019-2020 watamario")
    );

    hwnd = CreateWindowEx( // ここのExはこのままでいいみたい
        0, 
        WND_CLASS_NAME,
        TEXT("超大量ファイル整理"),
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_CLIPCHILDREN,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        480,
        320,
        NULL,
        NULL,
        hInstance,
        NULL);

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

    hbtn_ok = CreateWindowEx( // ok button
        0,
        TEXT("BUTTON"),
        TEXT("OK"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        0, 
        0,
        0,
        0,
        hwnd,
        (HMENU)1,
        hInstance,
        NULL);

    hbtn_abort = CreateWindowEx( // abort button
        0,
        TEXT("BUTTON"),
        TEXT("中断"),
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_DISABLED,
        0,
        0,
        0,
        0,
        hwnd,
        (HMENU)2,
        hInstance,
        NULL);

    hbtn_clr = CreateWindowEx( // clear the history button
        0,
        TEXT("BUTTON"),
        TEXT("履歴消去"),
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
    ShowWindow(hwnd, SW_MAXIMIZE);
    UpdateWindow(hwnd);

    MSG msg;
    BOOL bRet;
    SetTimer(hwnd, 1, TIMER_AWAIT, NULL);

    while( (bRet=GetMessage(&msg, hwnd, 0, 0)) ){ // continue unless the message is WM_QUIT(=0)
        if(bRet==-1) break;
        else{
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
    return (int)msg.wParam;
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch(uMsg) {
        case WM_CREATE: // called at first
            hCmdBar = CommandBar_Create(hInst, hWnd, 1);            
            CommandBar_InsertMenubar(hCmdBar, hInst, 1001, 0);
            CommandBar_Show(hCmdBar, TRUE);
            CmdBar_Height = CommandBar_Height(hCmdBar);

            hMemDC = CreateCompatibleDC(NULL);
            hBshSys = CreateSolidBrush(GetSysColor(COLOR_BTNFACE));
            hPenSys = CreatePen(PS_SOLID, 1, GetSysColor(COLOR_BTNFACE));
            
            hedi_path = CreateWindowEx( // 操作対象パス入力ボックス
               0,
               TEXT("EDIT"),
               NULL,
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
            wpedipath_old = (WNDPROC)SetWindowLong(hedi_path, GWL_WNDPROC, (DWORD)EditPathWindowProc);
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
            wpediname_old = (WNDPROC)SetWindowLong(hedi_name, GWL_WNDPROC, (DWORD)EditNameWindowProc);
            
            hedi_sepunit = CreateWindowEx( // 分割単位入力ボックス
                NULL,
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
            wpedisepunit_old = (WNDPROC)SetWindowLong(hedi_sepunit, GWL_WNDPROC, (DWORD)EditSepWindowProc);
            
            hedi_out = CreateWindowEx( // 出力表示ボックス
                0,
                TEXT("EDIT"),
                NULL,
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

        case WM_CLOSE: // called when closing
            KillTimer(hWnd, 1);
            DestroyWindow(hWnd);
            break;

        case WM_TIMER:
            if (b<=0 && g<255) g+=8; //
            if (g>=255 && r>0) r-=8; //
            if (r<=0 && b<255) b+=8; //
            if (b>=255 && g>0) g-=8; //
            if (g<=0 && r<255) r+=8; //
            if (r>=255 && b>0) b-=8; //
                                     //
            if (r>255) r=255;        //
            if (r<0) r=0;            //
            if (g>255) g=255;        //
            if (g<0) g=0;            //
            if (b>255) b=255;        //
            if (b<0) b=0;            //

            DeleteObject(hBrush);
            DeleteObject(hPen);
            hBrush = CreateSolidBrush(RGB(r, g, b)); // rgbブラシを作成(塗りつぶし用)
            hPen = CreatePen(PS_SOLID, 1, RGB(r, g, b)); // rgbペンを作成(輪郭用)
            InvalidateRect(hwnd, NULL, FALSE);
            
            if(!(hwnd_temp=GetFocus()) || (hwnd_temp!=hedi_path && hwnd_temp!=hedi_sepunit && hwnd_temp!=hedi_name)) break;
            hwnd_focused = hwnd_temp;
            break;

        case WM_SIZE: // when the window size is changed
            ResizeReplaceObjects();
            break;

        case WM_PAINT:
            Paint();
            break;

        case WM_ACTIVATE:
            // set focus on the input box when the main window is activated
            if(LOWORD(wParam) == WA_ACTIVE || LOWORD(wParam) == WA_CLICKACTIVE) SetFocus(hwnd_focused);
            break;

        case WM_COMMAND:
            switch(LOWORD(wParam)){
				case 0: // Select Directory
                    KillTimer(hWnd, 1);
					if(withdll) SelDir();
					else{
						MessageBox(
                            hWnd,
                            TEXT("ceshell.dllから必要な関数を読み込めなかったため、フォルダ選択ダイアログを使用できません｡\n")
						    TEXT("対象パスの欄に直接入力してください｡"),
                            TEXT("情報"),
                            MB_OK | MB_ICONINFORMATION);
                        SetTimer(hWnd, 1, TIMER_AWAIT, NULL);
					}
					break;

                case 1: // OK button
					if(working) break;
                    working = true;
                    KillTimer(hWnd, 1);
                    EnableWindow(hbtn_ok, FALSE);
                    EnableWindow(hedi_path, FALSE);
                    EnableWindow(hedi_name, FALSE);
                    EnableWindow(hedi_sepunit, FALSE);
                    EnableWindow(hbtn_abort, TRUE);

                    SendMessage(hedi_path, WM_GETTEXT, MAX_PATH, (LPARAM)InputBox.path);
                    SendMessage(hedi_name, WM_GETTEXT, MAX_PATH, (LPARAM)InputBox.name);
                    SendMessage(hedi_sepunit, WM_GETTEXT, 10, (LPARAM)tctemp);
                    InputBox.sepunit = _ttoi(tctemp);
                    InvalidateRect(hWnd, NULL, FALSE);

                    hThread = CreateThread(NULL, 0, SeparateFiles, 0, 0, &dThreadID);
                    SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
                    break;

                case 2: // abort
                    aborted = true;
                    break;

                case 3: // clear the history
                    editlen = SendMessage(hedi_out, WM_GETTEXTLENGTH, 0, 0);
                    SendMessage(hedi_out, EM_SETSEL, 0, editlen);
                    SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT(""));
                    break;

                case 2009: // close
                    SendMessage(hWnd, WM_CLOSE, 0, 0);
                    break;

                case 2020: // paste
                    SendMessage(hwnd_focused, WM_PASTE, 0, 0);
                    break;

				case 2101: // how to use
                    KillTimer(hWnd, 1);
                    TimerRestartable = false;
                    MessageBox(hWnd, tcmes[0], TEXT("使い方"), MB_OK | MB_ICONINFORMATION);
                    SetTimer(hWnd, 1, TIMER_AWAIT, NULL);
                    TimerRestartable = true;
                    break;

                case 2109: // about
                    KillTimer(hWnd, 1);
                    TimerRestartable = false;
                    MessageBox(hWnd, tcmes[1], TEXT("このプログラムについて"), MB_OK | MB_ICONINFORMATION);
                    SetTimer(hWnd, 1, TIMER_AWAIT, NULL);
                    TimerRestartable = true;
                    break;
            }
            if(LOWORD(wParam)<50) SetFocus(hwnd_focused); // set focus on input box if the clicked control isn't a edit box 
            else return DefWindowProc(hWnd, uMsg, wParam, lParam);
            break;

		case APP_SETFOCUS:
			SetFocus(hwnd_focused);
			break;
            
        case WM_DESTROY:
            CommandBar_Destroy(hCmdBar);
            PostQuitMessage(0);
            break;
            
        default:
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

// custom window procedure for the Edit Control
LRESULT CALLBACK EditPathWindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
    switch(uMsg){
        case WM_DESTROY:
            SetWindowLong(hWnd, GWL_WNDPROC, (DWORD)wpedipath_old);
            break;

        // Enterで次のボックスへ, 実行対象でなければdefaultに流す(間に別のメッセージ入れちゃだめ!!)
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
            SetWindowLong(hWnd, GWL_WNDPROC, (DWORD)wpediname_old);
            break;

        // Enterで次のボックスへ, 実行対象でなければdefaultに流す(間に別のメッセージ入れちゃだめ!!)
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
            SetWindowLong(hWnd, GWL_WNDPROC, (DWORD)wpedisepunit_old);
            break;

        // Enterで処理開始, 実行対象でなければdefaultに流す(間に別のメッセージ入れちゃだめ!!)
        case WM_CHAR:
            switch((CHAR)wParam){
                case VK_RETURN:
                    if(working) break;
                    working = true;
                    KillTimer(hWnd, 1);
                    EnableWindow(hbtn_ok, FALSE);
                    EnableWindow(hedi_path, FALSE);
                    EnableWindow(hedi_name, FALSE);
                    EnableWindow(hedi_sepunit, FALSE);
                    EnableWindow(hbtn_abort, TRUE);

                    SendMessage(hedi_path, WM_GETTEXT, MAX_PATH, (LPARAM)InputBox.path);
                    SendMessage(hedi_name, WM_GETTEXT, MAX_PATH, (LPARAM)InputBox.name);
                    SendMessage(hedi_sepunit, WM_GETTEXT, 10, (LPARAM)tctemp);
                    InputBox.sepunit = _ttoi(tctemp);
                    InvalidateRect(hWnd, NULL, FALSE);

                    hThread = CreateThread(NULL, 0, SeparateFiles, 0, 0, &dThreadID);
                    SetThreadPriority(hThread, THREAD_PRIORITY_BELOW_NORMAL);
                    return 0;                    
                case 'q':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("1"));
                    return 0;
                case 'w':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("2"));
                    return 0;
                case 'e':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("3"));
                    return 0;
                case 'r':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("4"));
                    return 0;
                case 't':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("5"));
                    return 0;
                case 'y':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("6"));
                    return 0;
                case 'u':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("7"));
                    return 0;
                case 'i':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("8"));
                    return 0;
                case 'o':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("9"));
                    return 0;
                case 'p':
                    SendMessage(hedi_sepunit, EM_REPLACESEL, 0, (WPARAM)TEXT("0"));
                    return 0;
            }
        default:
            return CallWindowProc(wpedisepunit_old, hWnd, uMsg, wParam, lParam);
    }
    return 0;
}

INT CALLBACK SelDirProc(HWND hWnd, UINT uMsg, LPARAM lParam1, LPARAM lParam2){
    switch (uMsg) {
        case BFFM_VALIDATEFAILED:
            MessageBox(hWnd, TEXT("不正なフォルダです"), TEXT("Error"), MB_OK | MB_ICONEXCLAMATION);
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
    bi.lpszTitle = TEXT("整理対象のフォルダを選択してください。");
    lpid = dll_SHBrowseForFolder(&bi);

    if(!lpid){
		SetTimer(hwnd, 1, TIMER_AWAIT, NULL);
		return;
    }else{
        hr = dll_SHGetMalloc(&pMalloc);
        if(hr == E_FAIL){
            MessageBox(hwnd, TEXT("SHGetMalloc()関数でエラーが発生しました"), TEXT("Error"), MB_OK | MB_ICONERROR);
            SetTimer(hwnd, 1, TIMER_AWAIT, NULL);
			return;
        }
        dll_SHGetPathFromIDList(lpid, szName);
        if(_tcscmp(szName, TEXT("\\"))==0) _tcscpy(szName, TEXT(""));
		SetWindowText(hedi_path, szName);
        pMalloc->Free(lpid);
        pMalloc->Release();
    }
	SetTimer(hwnd, 1, TIMER_AWAIT, NULL);
	SendMessage(hwnd, APP_SETFOCUS, 0, 0);
    return;
}

void ResizeReplaceObjects(){
    GetClientRect(hwnd, &rect);
    scrx = rect.right; scry = rect.bottom-CmdBar_Height; // Command Barの幅だけ引く

    // create a font for the main window
    if(scrx/20 < scry/10) rLogfont.lfHeight = scrx/20;
    else rLogfont.lfHeight = scry/10;
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
    
    // create a font for buttons
    if(24*scrx/700 < 24*scry/400) rLogfont.lfHeight = 24*scrx/700;
    else rLogfont.lfHeight = 24*scry/400;
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
    hFbtn=CreateFontIndirect(&rLogfont);
    
    // create a font for notes
    if(12*scrx/700 < 12*scry/400) rLogfont.lfHeight = 12*scrx/700;
    else rLogfont.lfHeight = 12*scry/400;
    DeleteObject(hFnote);
    hFnote = CreateFontIndirect(&rLogfont);

    // create a font for edit boxes
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

    // apply newly created fonts to objects
    SendMessage(hbtn_sel, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hbtn_ok, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hbtn_abort, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hedi_path, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hedi_out, WM_SETFONT, (WPARAM)hFedi, MAKELPARAM(FALSE, 0));
    SendMessage(hbtn_clr, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hedi_name, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));
    SendMessage(hedi_sepunit, WM_SETFONT, (WPARAM)hFbtn, MAKELPARAM(FALSE, 0));

    // move and resize objects
    MoveWindow(hCmdBar, NULL, NULL, NULL, NULL, TRUE);
    btnsize[0] = 64*scrx/700; btnsize[1] = 32*scry/400;
    MoveWindow(hedi_path, btnsize[0], CmdBar_Height, scrx-btnsize[0]*2, btnsize[1], TRUE);
    MoveWindow(hbtn_sel, scrx-btnsize[0], CmdBar_Height, btnsize[0], btnsize[1], TRUE);
    MoveWindow(hedi_name, btnsize[0], btnsize[1]+CmdBar_Height, btnsize[0]*2, btnsize[1], TRUE);
    MoveWindow(hedi_sepunit, btnsize[0]*4, btnsize[1]+CmdBar_Height, btnsize[0]*2, btnsize[1], TRUE);
    MoveWindow(hbtn_ok, btnsize[0]*6, btnsize[1]+CmdBar_Height, btnsize[0], btnsize[1], TRUE);
    MoveWindow(hbtn_abort, btnsize[0]*7, btnsize[1]+CmdBar_Height, btnsize[0], btnsize[1], TRUE);
    MoveWindow(hbtn_clr, btnsize[0]*8, btnsize[1]+CmdBar_Height, btnsize[0]*2, btnsize[1], TRUE);
    MoveWindow(hedi_out, scrx/20, scry*3/10+CmdBar_Height, scrx*9/10, scry*13/20, TRUE);

    hBitmap = CreateCompatibleBitmap(hdc=GetDC(hwnd), rect.right, rect.bottom);
    ReleaseDC(hwnd, hdc);
    SelectObject(hMemDC, hBitmap);
    DeleteObject(hBitmap);
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
    DrawText(hMemDC, TEXT("対象パス:"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    rect.top=btnsize[1]; rect.bottom=btnsize[1]*3/2;
    DrawText(hMemDC, TEXT("ファイル名"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    rect.top=btnsize[1]*3/2; rect.bottom=btnsize[1]*2;
    DrawText(hMemDC, TEXT("の形式:"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    rect.top=btnsize[1]; rect.left=btnsize[0]*3; rect.right=btnsize[0]*4;
    DrawText(hMemDC, TEXT("分割単位:"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    SetBkMode(hMemDC, OPAQUE);
    SetBkColor(hMemDC, RGB(255, 255, 0));
    SetTextColor(hMemDC, RGB(0, 0, 255));
	SelectObject(hMemDC, hMesFont);
    rect.left=0; rect.right=scrx; rect.top=btnsize[1]*2; rect.bottom=scry*3/10;
    if(working) DrawText(hMemDC, TEXT("整理中です..."), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    else DrawText(hMemDC, TEXT("大量のファイルを整理します"), -1, &rect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    rect.left=0; rect.right=scrx; rect.top=0; rect.bottom=scry;
    
    hdc = BeginPaint(hwnd, &ps);
    BitBlt(hdc, 0, CmdBar_Height, rect.right, rect.bottom, hMemDC, 0, 0, SRCCOPY);
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
        OutputToEditbox(hedi_out, TEXT("分割単位に無効な値が入力されました。\r\n"));
        SetWindowText(hwnd, TEXT("超大量ファイル整理"));
        if(TimerRestartable) SetTimer(hwnd, 1, TIMER_AWAIT, NULL);
        SendMessage(hwnd, APP_SETFOCUS, 0, 0);
        working=false;
        return 0;
    }

    wsprintf(tcsf, TEXT("%s\\%s"), InputBox.path, InputBox.name);
    hFFile = FindFirstFile(tcsf, &file);
    if(hFFile==INVALID_HANDLE_VALUE){ // エラー
        EnableWindow(hbtn_ok, TRUE);
        EnableWindow(hedi_path, TRUE);
        EnableWindow(hedi_name, TRUE);
        EnableWindow(hedi_sepunit, TRUE);
        EnableWindow(hbtn_abort, FALSE);
        OutputToEditbox(hedi_out, TEXT("該当するファイルが見つかりませんでした｡入力したパス､ファイル名の形式に誤りがないか確認してください｡\r\n"));
        SetWindowText(hwnd, TEXT("超大量ファイル整理"));
        if(TimerRestartable) SetTimer(hwnd, 1, TIMER_AWAIT, NULL);
        SendMessage(hwnd, APP_SETFOCUS, 0, 0);
		working=false;
        return 0;
    }

    wsprintf(tcsf, TEXT("%s\\1"), InputBox.path);
    CreateDirectory(tcsf, NULL);
    if(!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
        _tcscpy(tcresult, TEXT("現在1フォルダ目に整理中..."));
        SetWindowText(hwnd, tcresult);
        OutputToEditbox(hedi_out, tcresult);
        SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT("\r\n"));
        wsprintf(tcsf, TEXT("%s\\%s"), InputBox.path, file.cFileName);
        wsprintf(tcdest, TEXT("%s\\1\\%s"), InputBox.path, file.cFileName);
        if(!MoveFile(tcsf, tcdest)){
            wsprintf(tcresult, TEXT("ファイル%sを%sに移動中にエラーが発生しました｡ファイルを開いているアプリケーションは全て終了し､読み取り専用になっていないか確認して下さい｡\r\n"), tcsf, tcdest);
            OutputToEditbox(hedi_out, tcresult);
        }
        i++;
    }
    while(!aborted){
        if(FindNextFile(hFFile, &file)){
            if(!(file.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)){
                if(i==0){
                    _tcscpy(tcresult, TEXT("現在1フォルダ目に整理中..."));
                    SetWindowText(hwnd, tcresult);
                    OutputToEditbox(hedi_out, tcresult);
                    SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT("\r\n"));
                }
                if(i%InputBox.sepunit==0 && i!=0){
                    dirnum++;
                    wsprintf(tcsf, TEXT("%s\\%d"), InputBox.path, dirnum);
                    CreateDirectory(tcsf, NULL);
                    wsprintf(tcresult, TEXT("現在%dフォルダ目に整理中..."), dirnum);
                    SetWindowText(hwnd, tcresult);
                    OutputToEditbox(hedi_out, tcresult);
                    SendMessage(hedi_out, EM_REPLACESEL, 0, (WPARAM)TEXT("\r\n"));
                }
                wsprintf(tcsf, TEXT("%s\\%s"), InputBox.path, file.cFileName);
                wsprintf(tcdest, TEXT("%s\\%d\\%s"), InputBox.path, dirnum, file.cFileName);
                if(!MoveFile(tcsf, tcdest)){
                    wsprintf(tcresult, TEXT("ファイル%sを%sに移動中にエラーが発生しました｡ファイルを開いているアプリケーションは全て終了し､読み取り専用になっていないか確認して下さい｡\r\n"), tcsf, tcdest);
                    OutputToEditbox(hedi_out, tcresult);
                }
                i++;
            }
        }else{
            if(GetLastError()==ERROR_NO_MORE_FILES) _tcscpy(tcresult, TEXT("整理完了｡\r\n"));
            else _tcscpy(tcresult, TEXT("整理中に不明なエラーが発生しました｡\r\n"));
            break;
        }
    }

    FindClose(hFFile);
    SetWindowText(hwnd, TEXT("超大量ファイル整理"));

    if(aborted){
        aborted = false;
        _tcscpy(tcresult, TEXT("処理は中断されました｡\r\n"));
    }
    
    OutputToEditbox(hedi_out, tcresult);

    EnableWindow(hbtn_ok, TRUE);
    EnableWindow(hedi_path, TRUE);
    EnableWindow(hedi_name, TRUE);
    EnableWindow(hedi_sepunit, TRUE);
    EnableWindow(hbtn_abort, FALSE);
    InvalidateRect(hwnd, NULL, FALSE);
    if(TimerRestartable) SetTimer(hwnd, 1, TIMER_AWAIT, NULL);
    SendMessage(hwnd, APP_SETFOCUS, 0, 0);
	working=false;
    return 0;
}