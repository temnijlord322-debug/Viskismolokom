#include <windows.h>
#include <string>
#include <vector>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define ID_EDIT     1001
#define ID_OK       1002
#define ID_TIMER    2001

const char MAIN_CLASS[]  = "DemoMainWindow";
const char HELLO_CLASS[] = "DemoHelloWindow";
const char FLOOD_CLASS[] = "DemoFloodWindow";

HWND g_mainWindow = nullptr;
int  g_wrongAttempts = 0;
bool g_timerExpired  = false;

BOOL CALLBACK CollectOwnWindows(HWND hwnd, LPARAM lParam)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == GetCurrentProcessId())
    {
        auto* windows = reinterpret_cast<std::vector<HWND>*>(lParam);
        windows->push_back(hwnd);
    }
    return TRUE;
}

void CloseOwnWindows()
{
    std::vector<HWND> windows;
    EnumWindows(CollectOwnWindows, reinterpret_cast<LPARAM>(&windows));

    for (HWND hwnd : windows)
    {
        if (IsWindow(hwnd))
            DestroyWindow(hwnd);
    }
}

// Настоящие окна с символом 𰻞
LRESULT CALLBACK FloodWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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
        SetTextColor(hdc, RGB(0, 180, 0));

        const char* symbol = "𰻞";
        for (int i = 0; i < 1200; ++i)
        {
            RECT textRect = rc;
            textRect.top    = (i % 50) * 16;
            textRect.left   = ((i / 50) % 25) * 12;
            textRect.bottom = textRect.top + 16;
            textRect.right  = textRect.left + 16;

            DrawTextA(hdc, symbol, -1, &textRect, DT_LEFT | DT_SINGLELINE);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

LRESULT CALLBACK HelloWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
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

        for (int i = 0; i < 10; ++i)
        {
            RECT textRect = rc;
            textRect.top = 10 + i * 22;
            textRect.bottom = textRect.top + 22;

            DrawTextA(hdc, "Hello World!", -1, &textRect, DT_CENTER | DT_SINGLELINE);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

void OpenFloodWindows(HINSTANCE hInstance)
{
    const int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    const int FLOOD_COUNT = 1000;
    const int width  = 380;
    const int height = 280;

    for (int i = 0; i < FLOOD_COUNT; ++i)
    {
        int maxX = screenWidth  - width;
        int maxY = screenHeight - height;
        if (maxX < 1) maxX = 1;
        if (maxY < 1) maxY = 1;

        const int x = (i * 73) % maxX;
        const int y = (i * 47) % maxY;

        HWND flood = CreateWindowExA(
            WS_EX_APPWINDOW,
            FLOOD_CLASS,
            "𰻞 𰻞 𰻞",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            x, y, width, height,
            nullptr, nullptr, hInstance, nullptr
        );

        if (flood)
        {
            ShowWindow(flood, SW_SHOW);
            UpdateWindow(flood);
        }
    }
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        const int screenWidth = GetSystemMetrics(SM_CXSCREEN);

        CreateWindowA("STATIC", "ДЕМОНСТРАЦИОННОЕ ОКНО",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 160, screenWidth, 40, hwnd, nullptr, nullptr, nullptr);

        CreateWindowA("STATIC", "Введите пароль:",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 220, screenWidth, 40, hwnd, nullptr, nullptr, nullptr);

        HWND edit = CreateWindowExA(
            WS_EX_CLIENTEDGE, "EDIT", "",
            WS_VISIBLE | WS_CHILD | ES_PASSWORD | ES_CENTER,
            screenWidth / 2 - 150, 280, 300, 35,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_EDIT)),
            nullptr, nullptr);

        CreateWindowA("BUTTON", "РАЗБЛОКИРОВАТЬ",
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            screenWidth / 2 - 110, 340, 220, 45,
            hwnd, reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_OK)),
            nullptr, nullptr);

        if (edit)
            SetFocus(edit);

        // Таймер на 1 минуту
        SetTimer(hwnd, ID_TIMER, 60000, nullptr);

        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == ID_TIMER)
        {
            g_timerExpired = true;
            KillTimer(hwnd, ID_TIMER);
        }
        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_OK)
        {
            HWND edit = GetDlgItem(hwnd, ID_EDIT);
            char password[64] = {};

            if (edit)
                GetWindowTextA(edit, password, sizeof(password));

            if (std::string(password) == "3252")
            {
                CloseOwnWindows();
                PostQuitMessage(0);
            }
            else
            {
                if (edit)
                    SetWindowTextA(edit, "");

                g_wrongAttempts++;

                // После окончания таймера + 10 ошибок → 1000 настоящих окон
                if (g_timerExpired && g_wrongAttempts >= 10)
                {
                    HINSTANCE hInst = reinterpret_cast<HINSTANCE>(GetWindowLongPtr(hwnd, GWLP_HINSTANCE));
                    OpenFloodWindows(hInst);
                }
                else
                {
                    MessageBoxA(hwnd, "Неверный пароль!", "Ошибка", MB_OK | MB_ICONERROR);
                }
            }
            return 0;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        KillTimer(hwnd, ID_TIMER);
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    const int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // Регистрация классов
    WNDCLASSA mainClass = {};
    mainClass.lpfnWndProc   = MainWindowProc;
    mainClass.hInstance     = hInstance;
    mainClass.lpszClassName = MAIN_CLASS;
    mainClass.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_WINDOW + 1));
    mainClass.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    RegisterClassA(&mainClass);

    WNDCLASSA helloClass = {};
    helloClass.lpfnWndProc   = HelloWindowProc;
    helloClass.hInstance     = hInstance;
    helloClass.lpszClassName = HELLO_CLASS;
    helloClass.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_WINDOW + 1));
    helloClass.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    RegisterClassA(&helloClass);

    WNDCLASSA floodClass = {};
    floodClass.lpfnWndProc   = FloodWindowProc;
    floodClass.hInstance     = hInstance;
    floodClass.lpszClassName = FLOOD_CLASS;
    floodClass.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_WINDOW + 1));
    floodClass.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    RegisterClassA(&floodClass);

    g_mainWindow = CreateWindowExA(
        0, MAIN_CLASS, "Password Demo",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_mainWindow)
        return 0;

    // 40 обычных Hello-окон
    const int HELLO_COUNT = 40;
    for (int i = 0; i < HELLO_COUNT; ++i)
    {
        const int width  = 280;
        const int height = 180;

        int maxX = screenWidth  - width;
        int maxY = screenHeight - height;
        if (maxX < 1) maxX = 1;
        if (maxY < 1) maxY = 1;

        const int x = (i * 67) % maxX;
        const int y = (i * 43) % maxY;

        HWND hello = CreateWindowExA(
            WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_LAYERED,
            HELLO_CLASS, "",
            WS_POPUP | WS_VISIBLE,
            x, y, width, height,
            nullptr, nullptr, hInstance, nullptr);

        if (hello)
        {
            SetLayeredWindowAttributes(hello, 0, 190, LWA_ALPHA);
            SetWindowPos(hello, HWND_TOPMOST, x, y, width, height, SWP_SHOWWINDOW);
        }
    }

    MSG msg = {};
    while (GetMessageA(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    return 0;
}
