// language: C++, file: main.cpp, target: Windows 11, MSVC
// *fullscreen lock + hello world overlay + password release, closes all windows*
#include <windows.h>
#include <string>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define ID_EDIT 1001
#define ID_OK   1002

#define HELLO_CLASS "HelloOverlay"

static HHOOK g_kbHook = nullptr;
static HHOOK g_msHook = nullptr;
static HWND  g_lockWnd = nullptr;
static bool  g_unlocked = false;

// ---- пароль XOR ----
static const char ENC[] = {0x11,0x12,0x13,0x14}; // "3252" ^ 0x21
static const char KEY = 0x21;

std::string DecodePassword()
{
    std::string s;
    for (char c : ENC) s += (c ^ KEY);
    return s;
}

// ---- закрыть все окна процесса ----
BOOL CALLBACK CloseProcWindows(HWND hwnd, LPARAM)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == GetCurrentProcessId())
        DestroyWindow(hwnd);
    return TRUE;
}

// ---- оверлей Hello World: прозрачный для кликов, поверх всего ----
LRESULT CALLBACK HelloProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 255, 0));
        for (int i = 0; i < 40; ++i)
        {
            RECT line = rc;
            line.top = 5 + i * 22;
            line.bottom = line.top + 22;
            DrawTextA(hdc, "Hello World", -1, &line, DT_CENTER | DT_SINGLELINE);
        }
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_ERASEBKGND:
        return 1;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

// ---- keyboard hook ----
LRESULT CALLBACK KbProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        KBDLLHOOKSTRUCT* k = (KBDLLHOOKSTRUCT*)lParam;

        if (k->vkCode == VK_F4 && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (k->vkCode == VK_LWIN || k->vkCode == VK_RWIN) return 1;
        if (k->vkCode == VK_TAB && (GetAsyncKeyState(VK_MENU) & 0x8000)) return 1;
        if (k->vkCode == VK_ESCAPE)
        {
            if (GetAsyncKeyState(VK_MENU) & 0x8000) return 1;
            if (GetAsyncKeyState(VK_CONTROL) & 0x8000) return 1;
        }
        if (GetAsyncKeyState(VK_LWIN) & 0x8000) return 1;
        if (GetAsyncKeyState(VK_RWIN) & 0x8000) return 1;
    }
    return CallNextHookEx(g_kbHook, nCode, wParam, lParam);
}

// ---- mouse hook: только блок правого клика и кликов вне пароль-окна ----
LRESULT CALLBACK MsProc(int nCode, WPARAM wParam, LPARAM lParam)
{
    if (nCode == HC_ACTION)
    {
        if (wParam == WM_RBUTTONDOWN || wParam == WM_RBUTTONUP) return 1;

        MSLLHOOKSTRUCT* m = (MSLLHOOKSTRUCT*)lParam;
        POINT p = m->pt;
        RECT r;
        if (g_lockWnd && GetWindowRect(g_lockWnd, &r))
        {
            if (!PtInRect(&r, p)) return 1;
        }
    }
    return CallNextHookEx(g_msHook, nCode, wParam, lParam);
}

// ---- фокус-кипер: пароль-окно всегда активно, hello-окна поверх ----
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
        int sw = GetSystemMetrics(SM_CXSCREEN);
        int sh = GetSystemMetrics(SM_CYSCREEN);

        CreateWindowA("STATIC",
            "ВАШЕ УСТРОЙСТВО ЗАБЛОКИРОВАНО\n\nВведите пароль:",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 200, sw, 100,
            hwnd, nullptr, nullptr, nullptr);

        CreateWindowExA(WS_EX_CLIENTEDGE, "EDIT", "",
            WS_VISIBLE | WS_CHILD | ES_PASSWORD | ES_CENTER,
            sw/2 - 150, 320, 300, 32,
            hwnd, (HMENU)ID_EDIT, nullptr, nullptr);

        CreateWindowA("BUTTON", "РАЗБЛОКИРОВАТЬ",
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            sw/2 - 100, 370, 200, 40,
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
                if (g_kbHook) { UnhookWindowsHookEx(g_kbHook); g_kbHook = nullptr; }
                if (g_msHook) { UnhookWindowsHookEx(g_msHook); g_msHook = nullptr; }

                // закрыть ВСЕ окна процесса — включая hello-оверлеи
                EnumWindows(CloseProcWindows, 0);

                PostQuitMessage(0);
            }
            else
            {
                SetWindowTextA(GetDlgItem(hwnd, ID_EDIT), "");
                MessageBoxA(hwnd, "Неверный пароль", "Ошибка", MB_OK | MB_ICONERROR);
            }
            return 0;
        }
        break;

    case WM_CLOSE:
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInst, HINSTANCE, LPSTR, int)
{
    // ---- регистрируем оба класса ----
    WNDCLASSA wcLock = {};
    wcLock.lpfnWndProc = LockProc;
    wcLock.hInstance = hInst;
    wcLock.lpszClassName = "LockScreen";
    wcLock.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(10,10,10));
    wcLock.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wcLock);

    WNDCLASSA wcHello = {};
    wcHello.lpfnWndProc = HelloProc;
    wcHello.hInstance = hInst;
    wcHello.lpszClassName = HELLO_CLASS;
    wcHello.hbrBackground = nullptr; // без фона
    wcHello.hCursor = LoadCursor(nullptr, IDC_ARROW);
    RegisterClassA(&wcHello);

    int sw = GetSystemMetrics(SM_CXSCREEN);
    int sh = GetSystemMetrics(SM_CYSCREEN);

    // ---- 1) сначала пароль-окно (снизу) ----
    g_lockWnd = CreateWindowExA(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        "LockScreen", "",
        WS_POPUP | WS_VISIBLE,
        0, 0, sw, sh,
        nullptr, nullptr, hInst, nullptr);

    if (!g_lockWnd) return 1;

    // ---- 2) hello-оверлеи ПОВЕРХ, клики сквозь них ----
    const int HELLO_COUNT = 40;
    for (int i = 0; i < HELLO_COUNT; ++i)
    {
        int w = 300;
        int h = 250;
        int x = (i * 37) % (sw - w);
        int y = (i * 53) % (sh - h);

        HWND hw = CreateWindowExA(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW |
            WS_EX_TRANSPARENT | WS_EX_LAYERED,
            HELLO_CLASS, "",
            WS_POPUP | WS_VISIBLE,
            x, y, w, h,
            nullptr, nullptr, hInst, nullptr);

        if (hw)
        {
            // лёгкая прозрачность, чтобы пароль-окно читалось под ними
            SetLayeredWindowAttributes(hw, 0, 200, LWA_ALPHA);
        }
    }

    // ---- 3) хуки ----
    g_kbHook = SetWindowsHookEx(WH_KEYBOARD_LL, KbProc, hInst, 0);
    g_msHook = SetWindowsHookEx(WH_MOUSE_LL, MsProc, hInst, 0);

    // ---- 4) фокус-кипер ----
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
