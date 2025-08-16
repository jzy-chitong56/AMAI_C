#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>
#include <strsafe.h>

// 控件ID定义
#define IDC_MODE_COMBO       1001
#define IDC_BATCH_RADIO      1002
#define IDC_SINGLE_RADIO     1003
#define IDC_VERSION_COMBO    1004
#define IDC_PATH_EDIT        1005
#define IDC_BROWSE_BUTTON    1006
#define IDC_PROGRESS_BAR     1007
#define IDC_LOG_EDIT         1008
#define IDC_EXECUTE_BUTTON   1009

// 全局变量
HWND hMainWnd;
HWND hModeCombo, hBatchRadio, hSingleRadio, hVersionCombo;
HWND hPathEdit, hBrowseButton, hProgressBar, hLogEdit, hExecuteButton;
HFONT hFont = NULL;

// 模式定义
const LPCWSTR modes[] = {
    L"安装 AMAI (标准控制台)",
    L"安装 AMAI (对战暴雪AI控制台)",
    L"安装 AMAI (不安装控制台)",
    L"移除 AMAI 控制台",
    L"完全卸载 AMAI"
};

// 版本定义
const LPCWSTR versions[] = {
    L"重制版 (1.33+)",
    L"经典版: 冰封王座 (1.24e+)",
    L"经典版: 混乱之治 (1.24e-1.31)"
};

int currentMode = 0;

// 安全日志函数
void SafeAddLog(HWND hEdit, const WCHAR* message) {
    if (!hEdit || !message) return;
    
    int len = GetWindowTextLength(hEdit);
    SendMessage(hEdit, EM_SETSEL, len, len);
    SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)message);
    SendMessage(hEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

// 安全执行函数
BOOL SafeExecuteScript(const WCHAR* script, const WCHAR* args) {
    SHELLEXECUTEINFO sei = { sizeof(sei) };
    sei.hwnd = hMainWnd;
    sei.lpVerb = L"open";
    sei.lpFile = script;
    sei.lpParameters = args;
    sei.nShow = SW_HIDE;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS;

    if (!ShellExecuteEx(&sei)) {
        return FALSE;
    }

    if (sei.hProcess) {
        WaitForSingleObject(sei.hProcess, INFINITE);
        DWORD exitCode = 0;
        GetExitCodeProcess(sei.hProcess, &exitCode);
        CloseHandle(sei.hProcess);
        return (exitCode == 0);
    }
    return FALSE;
}

// 执行操作
void ExecuteOperation() {
    WCHAR path[MAX_PATH] = {0};
    GetWindowText(hPathEdit, path, MAX_PATH);
    
    if (path[0] == L'\0') {
        MessageBox(hMainWnd, L"请选择地图文件或文件夹", L"错误", MB_ICONERROR);
        return;
    }

    // 禁用UI
    EnableWindow(hModeCombo, FALSE);
    EnableWindow(hBatchRadio, FALSE);
    EnableWindow(hSingleRadio, FALSE);
    EnableWindow(hVersionCombo, FALSE);
    EnableWindow(hPathEdit, FALSE);
    EnableWindow(hBrowseButton, FALSE);
    EnableWindow(hExecuteButton, FALSE);

    SetWindowText(hLogEdit, L"");
    SafeAddLog(hLogEdit, L"开始操作...");

    int mode = currentMode;
    int version = SendMessage(hVersionCombo, CB_GETCURSEL, 0, 0);
    BOOL isBatch = SendMessage(hBatchRadio, BM_GETCHECK, 0, 0) == BST_CHECKED;

    // 构建安全参数
    WCHAR args[1024] = {0};
    LPCWSTR script = L"";

    switch (mode) {
    case 0: case 1: case 2:
        script = L"install.bat";
        StringCchPrintf(args, _countof(args), L"%d %d \"%s\"", 
                       (mode == 2) ? 0 : (mode == 1) ? 2 : 1,
                       version, path);
        break;
    case 3:
        script = L"uninstall_console.bat";
        StringCchPrintf(args, _countof(args), L"\"%s\"", path);
        break;
    case 4:
        script = L"uninstall_all.bat";
        StringCchPrintf(args, _countof(args), L"\"%s\"", path);
        break;
    }

    // 执行脚本
    SafeAddLog(hLogEdit, L"正在执行脚本...");
    if (SafeExecuteScript(script, args)) {
        SafeAddLog(hLogEdit, L"操作成功完成!");
    } else {
        WCHAR errorMsg[256];
        StringCchPrintf(errorMsg, _countof(errorMsg), 
                       L"执行失败: %s (错误 %d)", script, GetLastError());
        SafeAddLog(hLogEdit, errorMsg);
    }

    // 更新进度
    SendMessage(hProgressBar, PBM_SETPOS, 100, 0);

    // 启用UI
    EnableWindow(hModeCombo, TRUE);
    EnableWindow(hBatchRadio, TRUE);
    EnableWindow(hSingleRadio, TRUE);
    EnableWindow(hVersionCombo, TRUE);
    EnableWindow(hPathEdit, TRUE);
    EnableWindow(hBrowseButton, TRUE);
    EnableWindow(hExecuteButton, TRUE);
}

// 浏览路径
void BrowsePath() {
    BOOL isBatch = SendMessage(hBatchRadio, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    if (isBatch) {
        BROWSEINFO bi = {0};
        bi.hwndOwner = hMainWnd;
        bi.lpszTitle = L"选择地图文件夹";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        
        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl) {
            WCHAR path[MAX_PATH];
            if (SHGetPathFromIDList(pidl, path)) {
                SetWindowText(hPathEdit, path);
            }
            CoTaskMemFree(pidl);
        }
    } else {
        OPENFILENAME ofn = {0};
        WCHAR path[MAX_PATH] = {0};
        
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = hMainWnd;
        ofn.lpstrFilter = L"魔兽地图文件 (*.w3x;*.w3m)\0*.w3x;*.w3m\0所有文件 (*.*)\0*.*\0";
        ofn.lpstrFile = path;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
        
        if (GetOpenFileName(&ofn)) {
            SetWindowText(hPathEdit, path);
        }
    }
}

// 创建安全字体
HFONT CreateSafeFont() {
    LOGFONT lf = {0};
    lf.lfHeight = -12;
    lf.lfWeight = FW_NORMAL;
    lf.lfCharSet = DEFAULT_CHARSET;
    StringCchCopy(lf.lfFaceName, _countof(lf.lfFaceName), L"Microsoft YaHei");
    
    HFONT hFont = CreateFontIndirect(&lf);
    if (!hFont) {
        NONCLIENTMETRICS ncm = {0};
        ncm.cbSize = sizeof(ncm);
        SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0);
        hFont = CreateFontIndirect(&ncm.lfMessageFont);
    }
    return hFont;
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
    case WM_CREATE: {
        // 创建控件
        hModeCombo = CreateWindow(WC_COMBOBOX, L"", 
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
            20, 20, 300, 200, hWnd, (HMENU)IDC_MODE_COMBO, NULL, NULL);
        
        hBatchRadio = CreateWindow(WC_BUTTON, L"批量处理 (文件夹)", 
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP,
            20, 60, 150, 25, hWnd, (HMENU)IDC_BATCH_RADIO, NULL, NULL);
        
        hSingleRadio = CreateWindow(WC_BUTTON, L"单个地图", 
            WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
            180, 60, 100, 25, hWnd, (HMENU)IDC_SINGLE_RADIO, NULL, NULL);
        
        hVersionCombo = CreateWindow(WC_COMBOBOX, L"", 
            WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
            20, 100, 300, 200, hWnd, (HMENU)IDC_VERSION_COMBO, NULL, NULL);
        
        hPathEdit = CreateWindow(WC_EDIT, L"", 
            WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
            20, 140, 250, 25, hWnd, (HMENU)IDC_PATH_EDIT, NULL, NULL);
        
        hBrowseButton = CreateWindow(WC_BUTTON, L"浏览...", 
            WS_CHILD | WS_VISIBLE | WS_TABSTOP,
            280, 140, 80, 25, hWnd, (HMENU)IDC_BROWSE_BUTTON, NULL, NULL);
        
        hProgressBar = CreateWindow(PROGRESS_CLASS, L"", 
            WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
            20, 180, 340, 25, hWnd, (HMENU)IDC_PROGRESS_BAR, NULL, NULL);
        
        hLogEdit = CreateWindow(WC_EDIT, L"", 
            WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | 
            ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
            20, 220, 340, 150, hWnd, (HMENU)IDC_LOG_EDIT, NULL, NULL);
        
        hExecuteButton = CreateWindow(WC_BUTTON, L"执行", 
            WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_DEFPUSHBUTTON,
            20, 380, 340, 30, hWnd, (HMENU)IDC_EXECUTE_BUTTON, NULL, NULL);

        // 设置字体
        hFont = CreateSafeFont();
        if (hFont) {
            HWND childWindows[] = {
                hModeCombo, hBatchRadio, hSingleRadio, hVersionCombo,
                hPathEdit, hBrowseButton, hProgressBar, hLogEdit, hExecuteButton
            };
            for (HWND hwnd : childWindows) {
                SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, TRUE);
            }
        }

        // 初始化控件
        for (int i = 0; i < _countof(modes); i++) {
            SendMessage(hModeCombo, CB_ADDSTRING, 0, (LPARAM)modes[i]);
        }
        SendMessage(hModeCombo, CB_SETCURSEL, 0, 0);

        for (int i = 0; i < _countof(versions); i++) {
            SendMessage(hVersionCombo, CB_ADDSTRING, 0, (LPARAM)versions[i]);
        }
        SendMessage(hVersionCombo, CB_SETCURSEL, 0, 0);

        SendMessage(hBatchRadio, BM_SETCHECK, BST_CHECKED, 0);
        SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
        break;
    }
    case WM_COMMAND: {
        int wmId = LOWORD(wParam);
        int wmEvent = HIWORD(wParam);
        
        switch (wmId) {
        case IDC_MODE_COMBO:
            if (wmEvent == CBN_SELCHANGE) {
                currentMode = SendMessage(hModeCombo, CB_GETCURSEL, 0, 0);
            }
            break;
        case IDC_BROWSE_BUTTON:
            BrowsePath();
            break;
        case IDC_EXECUTE_BUTTON:
            ExecuteOperation();
            break;
        }
        break;
    }
    case WM_CLOSE:
        DestroyWindow(hWnd);
        break;
    case WM_DESTROY:
        if (hFont) DeleteObject(hFont);
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// 入口点
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow) {
    // 初始化通用控件
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_PROGRESS_CLASS | ICC_STANDARD_CLASSES };
    InitCommonControlsEx(&icc);

    // 注册窗口类
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = L"AMAIInstallerClass";
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, L"窗口注册失败!", L"错误", MB_ICONERROR);
        return 1;
    }

    // 创建主窗口
    hMainWnd = CreateWindowEx(0, L"AMAIInstallerClass", L"AMAI 安装工具", 
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 500, NULL, NULL, hInstance, NULL);

    if (!hMainWnd) {
        MessageBox(NULL, L"窗口创建失败!", L"错误", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (int)msg.wParam;
}
