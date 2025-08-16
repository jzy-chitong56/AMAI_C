#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker, "\"\"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"\"")

// 控件ID
#define IDC_COMBO_MODE      101
#define IDC_COMBO_MAP_PROC  102
#define IDC_COMBO_VERSION   103
#define IDC_EDIT_PATH       104
#define IDC_BUTTON_BROWSE   105
#define IDC_BUTTON_START    106
#define IDC_RADIO_FOLDER    107
#define IDC_RADIO_SINGLE    108

// 全局窗口句柄
HWND hMainWnd = NULL;
HWND hComboMode, hComboMapProc, hComboVersion;
HWND hEditPath, hRadioFolder, hRadioSingle;
HFONT hFont = NULL;

// 资源字符串（中文）
const char* WINDOW_TITLE = "AMAI 安装/卸载工具";
const char* LABEL_MODE = "请选择模式：";
const char* LABEL_MAP_PROC = "地图处理方式：";
const char* LABEL_VERSION = "请选择魔兽3版本：";
const char* LABEL_PATH = "地图路径或文件：";
const char* BTN_BROWSE = "浏览...";
const char* BTN_START = "开始执行";
const char* MODE_ITEMS[] = {
    "1. 安装 AMAI (常规控制台)",
    "2. 安装 AMAI (VS暴雪AI控制台)",
    "3. 安装 AMAI (不安装控制台)",
    "4. 移除 AMAI 控制台",
    "5. 卸载 AMAI 及控制台"
};
const char* VERSION_ITEMS[] = {
    "1. 重制版 REFORGED (1.33+)",
    "2. 经典版-冰封王座 (1.24e+)",
    "3. 经典版-混乱之治 (1.24e ~ 1.31)"
};

// 生成批处理命令并执行
void ExecuteCommand() {
    char cmd[4096] = {0};
    char buffer[512] = {0};

    // 获取模式选择
    int mode_index = SendMessageA(hComboMode, CB_GETCURSEL, 0, 0);
    if (mode_index == CB_ERR) mode_index = 0;  // 默认第一个
    const char* command_str;
    switch (mode_index) {
        case 0: command_str = "1"; break;
        case 1: command_str = "2"; break;
        case 2: command_str = "0"; break;
        case 3: command_str = "-1"; break;
        case 4: command_str = "-2"; break;
        default: command_str = "1";
    }

    // 获取版本选择
    int ver_index = SendMessageA(hComboVersion, CB_GETCURSEL, 0, 0);
    if (ver_index == CB_ERR) ver_index = 0;
    const char* ver_str;
    switch (ver_index) {
        case 0: ver_str = "REFORGED"; break;
        case 1: ver_str = "TFT"; break;
        case 2: ver_str = "ROC"; break;
        default: ver_str = "REFORGED";
    }

    // 获取路径
    GetWindowTextA(hEditPath, buffer, sizeof(buffer) - 1);
    if (strlen(buffer) == 0) {
        MessageBoxA(hMainWnd, "请输入地图路径或文件！", "错误", MB_ICONERROR);
        return;
    }

    // 检查路径是否存在
    DWORD attr = GetFileAttributesA(buffer);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        MessageBoxA(hMainWnd, "路径或文件不存在！", "错误", MB_ICONERROR);
        return;
    }

    // 判断是文件夹还是文件
    int is_folder = (attr & FILE_ATTRIBUTE_DIRECTORY) ? 1 : 0;
    if (IsDlgButtonChecked(hMainWnd, IDC_RADIO_SINGLE)) is_folder = 0;

    // 构造调用外部批处理的命令
    // 我们将原始批处理重命名为 AMAI_Installer.bat，并通过参数传递选项
    snprintf(cmd, sizeof(cmd),
        "call \"AMAI_Installer.bat\" \"%s\" \"%s\" \"%s\" \"%s\"",
        command_str, ver_str, is_folder ? "1" : "0", buffer);

    // 显示执行命令（调试用）
    char msg[1024];
    snprintf(msg, sizeof(msg), "即将执行:\n%s\n\n注意：请确保 AMAI_Installer.bat 存在于本程序同目录下。", cmd);
    MessageBoxA(hMainWnd, msg, "执行命令", MB_OK | MB_ICONINFORMATION);

    // 执行批处理（隐藏窗口）
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;

    if (!CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        MessageBoxA(hMainWnd, "无法启动批处理脚本！请确认 AMAI_Installer.bat 存在。", "错误", MB_ICONERROR);
        return;
    }

    // 等待完成（可选：改为异步执行并提示完成）
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    MessageBoxA(hMainWnd, "操作已完成。", "完成", MB_OK | MB_ICONINFORMATION);
}

// 浏览文件夹或文件
void BrowsePath() {
    OPENFILENAMEA ofn = {0};
    char szFile[512] = {0};

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = hMainWnd;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = sizeof(szFile);
    ofn.lpstrFilter = "地图文件 (*.w3m;*.w3x)\0*.w3m;*.w3x\0所有文件 (*.*)\0*.*\0";
    ofn.nFilterIndex = 1;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

    if (IsDlgButtonChecked(hMainWnd, IDC_RADIO_FOLDER)) {
        ofn.Flags |= OFN_PICKFOLDERS;
        ofn.lpstrTitle = "选择地图文件夹";
        if (SHBrowseForFolderA(&ofn)) {
            SetWindowTextA(hEditPath, szFile);
        }
    } else {
        ofn.lpstrTitle = "选择地图文件";
        if (GetOpenFileNameA(&ofn)) {
            SetWindowTextA(hEditPath, szFile);
        }
    }
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_CREATE: {
            HFONT hDefaultFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

            // 标签：模式
            CreateWindowExA(0, "STATIC", LABEL_MODE, WS_CHILD | WS_VISIBLE, 20, 20, 200, 20, hwnd, NULL, NULL, NULL);
            hComboMode = CreateWindowExA(0, "COMBOBOX", "", 
                CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                20, 45, 300, 150, hwnd, (HMENU)IDC_COMBO_MODE, NULL, NULL);
            SendMessageA(hComboMode, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
            for (int i = 0; i < 5; i++) {
                SendMessageA(hComboMode, CB_ADDSTRING, 0, (LPARAM)MODE_ITEMS[i]);
            }
            SendMessageA(hComboMode, CB_SETCURSEL, 0, 0);

            // 标签：处理方式
            CreateWindowExA(0, "STATIC", LABEL_MAP_PROC, WS_CHILD | WS_VISIBLE, 20, 80, 200, 20, hwnd, NULL, NULL, NULL);
            hRadioFolder = CreateWindowExA(0, "BUTTON", "按文件夹批量处理", 
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON, 20, 105, 150, 20, hwnd, (HMENU)IDC_RADIO_FOLDER, NULL, NULL);
            hRadioSingle = CreateWindowExA(0, "BUTTON", "逐张地图处理", 
                WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON, 180, 105, 150, 20, hwnd, (HMENU)IDC_RADIO_SINGLE, NULL, NULL);
            SendMessageA(hRadioFolder, BM_SETCHECK, BST_CHECKED, 0);

            // 标签：版本
            CreateWindowExA(0, "STATIC", LABEL_VERSION, WS_CHILD | WS_VISIBLE, 20, 140, 200, 20, hwnd, NULL, NULL, NULL);
            hComboVersion = CreateWindowExA(0, "COMBOBOX", "", 
                CBS_DROPDOWNLIST | WS_CHILD | WS_VISIBLE | WS_VSCROLL,
                20, 165, 300, 150, hwnd, (HMENU)IDC_COMBO_VERSION, NULL, NULL);
            SendMessageA(hComboVersion, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);
            for (int i = 0; i < 3; i++) {
                SendMessageA(hComboVersion, CB_ADDSTRING, 0, (LPARAM)VERSION_ITEMS[i]);
            }
            SendMessageA(hComboVersion, CB_SETCURSEL, 0, 0);

            // 路径输入
            CreateWindowExA(0, "STATIC", LABEL_PATH, WS_CHILD | WS_VISIBLE, 20, 200, 200, 20, hwnd, NULL, NULL, NULL);
            hEditPath = CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "", 
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                20, 225, 380, 25, hwnd, (HMENU)IDC_EDIT_PATH, NULL, NULL);
            SendMessageA(hEditPath, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);

            // 浏览按钮
            HWND hBtnBrowse = CreateWindowExA(0, "BUTTON", BTN_BROWSE, 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                410, 225, 80, 25, hwnd, (HMENU)IDC_BUTTON_BROWSE, NULL, NULL);
            SendMessageA(hBtnBrowse, WM_SETFONT, (WPARAM)hDefaultFont, TRUE);

            // 开始按钮
            HWND hBtnStart = CreateWindowExA(0, "BUTTON", BTN_START, 
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
                200, 270, 100, 35, hwnd, (HMENU)IDC_BUTTON_START, NULL, NULL);
            hFont = CreateFontA(16, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, 0, "Microsoft YaHei");
            SendMessageA(hBtnStart, WM_SETFONT, (WPARAM)(hFont ? hFont : hDefaultFont), TRUE);

            break;
        }
        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDC_BUTTON_BROWSE:
                    BrowsePath();
                    break;
                case IDC_BUTTON_START:
                    ExecuteCommand();
                    break;
            }
            break;

        case WM_DESTROY:
            if (hFont) DeleteObject(hFont);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProcA(hwnd, msg, wParam, lParam);
    }
    return 0;
}

// WinMain
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char* CLASS_NAME = "AMAI_GUI_Class";

    WNDCLASSEXA wc = {0};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExA(&wc)) {
        MessageBoxA(NULL, "窗口类注册失败！", "错误", MB_ICONERROR);
        return 1;
    }

    hMainWnd = CreateWindowExA(
        0, CLASS_NAME, WINDOW_TITLE,
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 520, 360,
        NULL, NULL, hInstance, NULL);

    if (!hMainWnd) {
        MessageBoxA(NULL, "窗口创建失败！", "错误", MB_ICONERROR);
        return 1;
    }

    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);

    MSG msg;
    while (GetMessageA(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return (int)msg.wParam;
}
