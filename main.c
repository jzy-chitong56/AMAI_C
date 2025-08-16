#include <windows.h>
#include <commctrl.h>
#include <shlobj.h>

#pragma comment(lib, "comctl32.lib")

// 全局变量
HWND hMainWnd, hPathEdit, hMapEdit;
int commandMode = 1;
int processMode = 1;
wchar_t version[32] = L"REFORGED";

// 控件ID
#define IDC_MODE_COMBO    1001
#define IDC_PROCESS_COMBO 1002
#define IDC_VER_COMBO     1003
#define IDC_BROWSE_BTN    1004
#define IDC_EXECUTE_BTN   1005

// 初始化控件
void InitControls(HWND hWnd)
{
    // 模式选择下拉框
    CreateWindowW(L"ComboBox", L"", 
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        20, 20, 300, 150, hWnd, (HMENU)IDC_MODE_COMBO, NULL, NULL);
    
    // 添加选项（需转换为中文）
    SendMessageW(GetDlgItem(hWnd, IDC_MODE_COMBO), CB_ADDSTRING, 0, (LPARAM)L"安装 AMAI (常规控制台)");
    // ...其他选项类似

    // 版本选择
    CreateWindowW(L"ComboBox", L"", 
        WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST | CBS_HASSTRINGS,
        20, 60, 300, 150, hWnd, (HMENU)IDC_VER_COMBO, NULL, NULL);
    SendMessageW(GetDlgItem(hWnd, IDC_VER_COMBO), CB_ADDSTRING, 0, (LPARAM)L"重制版 REFORGED (1.33+)");
    // ...其他版本

    // 路径输入框
    hPathEdit = CreateWindowW(L"Edit", L"", 
        WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
        20, 100, 250, 25, hWnd, NULL, NULL, NULL);
    
    // 浏览按钮
    CreateWindowW(L"Button", L"浏览...", 
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
        280, 100, 80, 25, hWnd, (HMENU)IDC_BROWSE_BTN, NULL, NULL);
}

// 执行命令函数
void ExecuteCommand()
{
    wchar_t cmd[1024];
    wchar_t path[MAX_PATH];
    
    // 获取路径
    GetWindowTextW(hPathEdit, path, MAX_PATH);
    
    // 构建命令
    swprintf_s(cmd, _countof(cmd), L"InstallVERToMap.exe %s \"%s\" %d", version, path, commandMode);

    // 执行外部命令
    STARTUPINFO si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    CreateProcessW(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    
    // 等待完成
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hProcess);
    
    MessageBoxW(hMainWnd, L"操作完成！", L"提示", MB_OK);
}

// 窗口过程
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDC_BROWSE_BTN:
        {
            BROWSEINFOW bi = { 0 };
            bi.hwndOwner = hWnd;
            bi.lpszTitle = L"选择地图文件夹";
            LPITEMIDLIST pidl = SHBrowseForFolderW(&bi);
            if (pidl)
            {
                wchar_t path[MAX_PATH];
                SHGetPathFromIDListW(pidl, path);
                SetWindowTextW(hPathEdit, path);
                CoTaskMemFree(pidl);
            }
            break;
        }
        case IDC_EXECUTE_BTN:
            ExecuteCommand();
            break;
        }
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

// 主函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 初始化通用控件
    INITCOMMONCONTROLSEX icc = { sizeof(icc), ICC_WIN95_CLASSES };
    InitCommonControlsEx(&icc);
    
    // 注册窗口类
    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = L"AMAIInstaller";
    RegisterClassExW(&wc);
    
    // 创建主窗口
    hMainWnd = CreateWindowW(L"AMAIInstaller", L"AMAI 安装工具",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 400, 300,
        NULL, NULL, hInstance, NULL);
    
    // 初始化控件
    InitControls(hMainWnd);
    
    ShowWindow(hMainWnd, nCmdShow);
    UpdateWindow(hMainWnd);
    
    // 消息循环
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    return (int)msg.wParam;
}
