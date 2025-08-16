#define UNICODE
#define _UNICODE

#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>
#include <stdio.h>
#include <tchar.h>

// 控件ID定义
#define IDC_MODE_COMBO       1001
#define IDC_BATCH_RADIO       1002
#define IDC_SINGLE_RADIO      1003
#define IDC_VERSION_COMBO     1004
#define IDC_PATH_EDIT         1005
#define IDC_BROWSE_BUTTON     1006
#define IDC_PROGRESS_BAR      1007
#define IDC_LOG_EDIT          1008
#define IDC_EXECUTE_BUTTON    1009

// 全局变量
HWND hMainWnd;
HWND hModeCombo, hBatchRadio, hSingleRadio, hVersionCombo;
HWND hPathEdit, hBrowseButton, hProgressBar, hLogEdit, hExecuteButton;
HFONT hFont; // 全局字体

// 模式定义
const TCHAR* modes[] = {
    L"安装 AMAI (标准控制台)",
    L"安装 AMAI (对战暴雪AI控制台)",
    L"安装 AMAI (不安装控制台)",
    L"移除 AMAI 控制台",
    L"完全卸载 AMAI"
};

// 版本定义
const TCHAR* versions[] = {
    L"重制版 (1.33+)",
    L"经典版: 冰封王座 (1.24e+)",
    L"经典版: 混乱之治 (1.24e-1.31)"
};

// 当前选择的模式
int currentMode = 0;

// 日志添加函数（宽字符安全）
void AddLog(const TCHAR* message) {
    if (!hLogEdit) return; // 确保日志框有效

    int len = GetWindowTextLength(hLogEdit);
    SendMessage(hLogEdit, EM_SETSEL, len, len);
    SendMessage(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)message);
    SendMessage(hLogEdit, EM_REPLACESEL, FALSE, (LPARAM)L"\r\n");
}

// 执行操作（修正字符串格式化和安全检查）
void ExecuteOperation() {
    TCHAR path[MAX_PATH] = {0};
    GetWindowText(hPathEdit, path, MAX_PATH);
    
    if (_tcslen(path) == 0) {
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
    
    // 清空日志
    SetWindowText(hLogEdit, L"");
    AddLog(L"开始操作...");
    
    // 获取模式和版本
    int mode = currentMode;
    int version = SendMessage(hVersionCombo, CB_GETCURSEL, 0, 0);
    
    // 获取处理方式（批量/单个）
    BOOL isBatch = SendMessage(hBatchRadio, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    // 构建参数（使用安全字符串函数）
    TCHAR args[1024] = {0};
    TCHAR script[50] = {0};
    
    if (mode < 3) { // 安装模式
        _tcscpy_s(script, L"install.bat"); // 使用安全拷贝
        
        // 控制台类型: 0=不安装, 1=标准, 2=VS暴雪
        int consoleType = (mode == 2) ? 0 : (mode == 1) ? 2 : 1;
        
        // 版本参数
        _stprintf_s(args, L"%d %d \"%s\"", consoleType, version, path);
    } else if (mode == 3) { // 移除控制台
        _tcscpy_s(script, L"uninstall_console.bat");
        _stprintf_s(args, L"\"%s\"", path);
    } else { // 完全卸载
        _tcscpy_s(script, L"uninstall_all.bat");
        _stprintf_s(args, L"\"%s\"", path);
    }
    
    // 执行批处理（安全命令行构建）
    AddLog(L"正在执行脚本...");
    
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    TCHAR commandLine[1024] = {0};
    
    // 使用swprintf_s安全格式化命令行
    if (swprintf_s(commandLine, L"cmd.exe /C \"%s %s\"", script, args) < 0) {
        AddLog(L"错误：命令行过长或格式错误");
        goto Cleanup; // 跳转到清理逻辑
    }
    
    if (CreateProcess(NULL, commandLine, NULL, NULL, FALSE, 
                     CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        AddLog(L"脚本执行完成!");
    } else {
        TCHAR errorMsg[256];
        _stprintf_s(errorMsg, L"执行失败: %s (错误 %d)", script, GetLastError());
        AddLog(errorMsg);
    }

Cleanup:
    // 更新进度
    SendMessage(hProgressBar, PBM_SETPOS, 100, 0);
    AddLog(L"操作完成!");
    
    // 启用UI
    EnableWindow(hModeCombo, TRUE);
    EnableWindow(hBatchRadio, TRUE);
    EnableWindow(hSingleRadio, TRUE);
    EnableWindow(hVersionCombo, TRUE);
    EnableWindow(hPathEdit, TRUE);
    EnableWindow(hBrowseButton, TRUE);
    EnableWindow(hExecuteButton, TRUE);
}

// 浏览文件或文件夹（修正路径处理）
void BrowsePath() {
    if (!IsWindow(hPathEdit)) return; // 确保路径框有效

    BOOL isBatch = SendMessage(hBatchRadio, BM_GETCHECK, 0, 0) == BST_CHECKED;
    
    if (isBatch) {
        // 浏览文件夹
        BROWSEINFO bi = {0};
        bi.hwndOwner = hMainWnd;
        bi.lpszTitle = L"选择地图文件夹";
        bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
        
        LPITEMIDLIST pidl = SHBrowseForFolder(&bi);
        if (pidl) {
            TCHAR path[MAX_PATH] = {0};
            if (SHGetPathFromIDList(pidl, path)) {
                SetWindowText(hPathEdit, path);
            }
            CoTaskMemFree(pidl);
        }
    } else {
        // 浏览文件
        OPENFILENAME ofn = {0};
        TCHAR path[MAX_PATH] = {0};
        
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

// 窗口过程函数（优化字体和控件创建）
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            // 创建控件
            hModeCombo = CreateWindow(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                20, 20, 300, 200, hWnd, (HMENU)IDC_MODE_COMBO, NULL, NULL);
            hBatchRadio = CreateWindow(L"BUTTON", L"批量处理 (文件夹)", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                20, 60, 150, 25, hWnd, (HMENU)IDC_BATCH_RADIO, NULL, NULL);
            hSingleRadio = CreateWindow(L"BUTTON", L"单个地图", WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                180, 60, 100, 25, hWnd, (HMENU)IDC_SINGLE_RADIO, NULL, NULL);
            hVersionCombo = CreateWindow(L"COMBOBOX", L"", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | WS_TABSTOP,
                20, 100, 300, 200, hWnd, (HMENU)IDC_VERSION_COMBO, NULL, NULL);
            hPathEdit = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                20, 140, 250, 25, hWnd, (HMENU)IDC_PATH_EDIT, NULL, NULL);
            hBrowseButton = CreateWindow(L"BUTTON", L"浏览...", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                280, 140, 80, 25, hWnd, (HMENU)IDC_BROWSE_BUTTON, NULL, NULL);
            hProgressBar = CreateWindow(PROGRESS_CLASS, L"", WS_CHILD | WS_VISIBLE | PBS_SMOOTH,
                20, 180, 340, 25, hWnd, (HMENU)IDC_PROGRESS_BAR, NULL, NULL);
            hLogEdit = CreateWindow(L"EDIT", L"", WS_CHILD | WS_VISIBLE | WS_BORDER | WS_VSCROLL | 
                ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
                20, 220, 340, 150, hWnd, (HMENU)IDC_LOG_EDIT, NULL, NULL);
            hExecuteButton = CreateWindow(L"BUTTON", L"执行", WS_CHILD | WS_VISIBLE | WS_TABSTOP,
                20, 380, 340, 30, hWnd, (HMENU)IDC_EXECUTE_BUTTON, NULL, NULL);

            // 检查控件是否创建成功
            if (!hModeCombo || !hBatchRadio || !hSingleRadio || !hVersionCombo || 
                !hPathEdit || !hBrowseButton || !hProgressBar || !hLogEdit || !hExecuteButton) {
                MessageBox(hWnd, L"控件创建失败!", L"错误", MB_ICONERROR);
                return -1;
            }

            // 设置字体（使用系统默认字符集）
            NONCLIENTMETRICS ncm = {0};
            ncm.cbSize = sizeof(NONCLIENTMETRICS);
            SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(NONCLIENTMETRICS), &ncm, 0);
            
            LOGFONT lf = {0};
            lf.lfHeight = -MulDiv(10, GetDeviceCaps(GetDC(NULL), LOGPIXELSY), 72);
            lf.lfWeight = FW_NORMAL;
            wcscpy_s(lf.lfFaceName, LF_FACESIZE, L"微软雅黑"); // 使用微软雅黑
            lf.lfCharSet = DEFAULT_CHARSET; // 关键修改：使用系统默认字符集

            hFont = CreateFontIndirect(&lf);
            if (!hFont) {
                // 回退到系统消息字体
                hFont = CreateFontIndirect(&ncm.lfMessageFont);
            }

            if (hFont) {
                SendMessage(hModeCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
                SendMessage(hBatchRadio, WM_SETFONT, (WPARAM)hFont, TRUE);
                SendMessage(hSingleRadio, WM_SETFONT, (WPARAM)hFont, TRUE);
                SendMessage(hVersionCombo, WM_SETFONT, (WPARAM)hFont, TRUE);
                SendMessage(hPathEdit, WM_SETFONT, (WPARAM)hFont, TRUE);
                SendMessage(hBrowseButton, WM_SETFONT, (WPARAM)hFont, TRUE);
                SendMessage(hProgressBar, WM_SETFONT, (WPARAM)hFont, TRUE); // 进度条可能不需要，但无影响
                SendMessage(hLogEdit, WM_SETFONT, (WPARAM)hFont, TRUE); // 关键：日志框字体
                SendMessage(hExecuteButton, WM_SETFONT, (WPARAM)hFont, TRUE);
            }

            // 填充组合框
            for (int i = 0; i < _countof(modes); i++) {
                SendMessage(hModeCombo, CB_ADDSTRING, 0, (LPARAM)modes[i]);
            }
            SendMessage(hModeCombo, CB_SETCURSEL, 0, 0);

            for (int i = 0; i < _countof(versions); i++) {
                SendMessage(hVersionCombo, CB_ADDSTRING, 0, (LPARAM)versions[i]);
            }
            SendMessage(hVersionCombo, CB_SETCURSEL, 0, 0);

            // 默认选择批量处理
            SendMessage(hBatchRadio, BM_SETCHECK, BST_CHECKED, 0);

            // 进度条初始化
            SendMessage(hProgressBar, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
            SendMessage(hProgressBar, PBM_SETPOS, 0, 0);

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

// 主函数（移除无关控制台编码设置）
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
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
    hMainWnd = CreateWindowEx(0, wc.lpszClassName, L"AMAI 安装工具", 
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 400, 500, NULL, NULL, hInstance, NULL);
    
    if (!hMainWnd) {
        MessageBox(NULL, L"窗口创建失败!", L"错误", MB_ICONERROR);
        return 1;
    }
    
    // 初始化通用控件（进度条需要）
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_PROGRESS_CLASS };
    InitCommonControlsEx(&icc);
    
    // 显示窗口
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
