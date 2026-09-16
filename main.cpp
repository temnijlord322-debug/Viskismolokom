#include <windows.h>
#include <string>
#include <vector>
#include <gdiplus.h>
#include <ctime>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

#define ID_EDIT          1001
#define ID_OK            1002
#define ID_TIMER         2001
#define ID_LOADING_TIMER 2002
#define ID_ANIM_TIMER    2003

const char MAIN_CLASS[]     = "DemoMainWindow";
const char HELLO_CLASS[]    = "DemoHelloWindow";
const wchar_t FLOOD_CLASS[] = L"DemoFloodWindow";

HWND g_mainWindow = nullptr;
int  g_wrongAttempts = 0;
bool g_timerExpired  = false;
bool g_loading       = true;
bool g_floodOpened   = false;

Image* g_skullImage = nullptr;
ULONG_PTR g_gdiplusToken = 0;

struct Drop {
    int x;
    float y;
    float speed;
    int length;
};

std::vector<Drop> g_drops;
const int DROP_COUNT = 280;

void InitDrops(int screenWidth, int screenHeight)
{
    g_drops.clear();
    srand((unsigned)time(nullptr));

    for (int i = 0; i < DROP_COUNT; ++i)
    {
        Drop d;
        d.x = rand() % screenWidth;
        d.y = (float)(-(rand() % (screenHeight + 400)));
        d.speed = 2.5f + (rand() % 10);
        d.length = 12 + rand() % 28;
        g_drops.push_back(d);
    }
}

void UpdateDrops(int screenWidth, int screenHeight)
{
    for (auto& d : g_drops)
    {
        d.y += d.speed;
        if (d.y > screenHeight + 60)
        {
            d.y = (float)(-60 - rand() % 400);
            d.x = rand() % screenWidth;
            d.speed = 2.5f + (rand() % 10);
        }
    }
}

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
        if (IsWindow(hwnd))
            DestroyWindow(hwnd);
}

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

        HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rc, black);
        DeleteObject(black);

        SetBkMode(hdc, TRANSPARENT);
        SetTextColor(hdc, RGB(0, 220, 0));

        const wchar_t* symbol = L"𰻞";
        for (int i = 0; i < 1600; ++i)
        {
            RECT textRect = rc;
            textRect.top    = (i % 55) * 15;
            textRect.left   = ((i / 55) % 30) * 11;
            textRect.bottom = textRect.top + 15;
            textRect.right  = textRect.left + 15;
            DrawTextW(hdc, symbol, -1, &textRect, DT_LEFT | DT_SINGLELINE);
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
    return DefWindowProcW(hwnd, msg, wParam, lParam);
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

        HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rc, black);
        DeleteObject(black);

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
    if (g_floodOpened)
        return;
    g_floodOpened = true;

    const int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    const int FLOOD_COUNT = 1000;
    const int width  = 400;
    const int height = 300;

    for (int i = 0; i < FLOOD_COUNT; ++i)
    {
        int maxX = screenWidth  - width;
        int maxY = screenHeight - height;
        if (maxX < 1) maxX = 1;
        if (maxY < 1) maxY = 1;

        const int x = (i * 79) % maxX;
        const int y = (i * 53) % maxY;

        HWND flood = CreateWindowExW(
            WS_EX_APPWINDOW | WS_EX_TOPMOST,
            FLOOD_CLASS,
            L"𰻞 𰻞 𰻞 𰻞 𰻞",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            x, y, width, height,
            nullptr, nullptr, hInstance, nullptr);

        if (flood)
        {
            ShowWindow(flood, SW_SHOW);
            UpdateWindow(flood);
        }
    }
}

void TryOpenFlood(HWND hwnd)
{
    if (g_timerExpired && g_wrongAttempts >= 6)
    {
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
        if (!hInst)
            hInst = GetModuleHandleA(nullptr);
        OpenFloodWindows(hInst);
    }
}

void CreatePasswordUI(HWND hwnd)
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
}

void CreateHelloWindows(HINSTANCE hInstance)
{
    const int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    for (int i = 0; i < 40; ++i)
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
}

LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        const int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
        const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

        InitDrops(screenWidth, screenHeight);

        g_skullImage = new Image(L"skull.jpg");
        if (g_skullImage->GetLastStatus() != Ok)
        {
            delete g_skullImage;
            g_skullImage = nullptr;
        }

        SetTimer(hwnd, ID_LOADING_TIMER, 5000, nullptr);
        SetTimer(hwnd, ID_ANIM_TIMER, 25, nullptr);
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == ID_LOADING_TIMER)
        {
            KillTimer(hwnd, ID_LOADING_TIMER);
            KillTimer(hwnd, ID_ANIM_TIMER);
            g_loading = false;

            CreatePasswordUI(hwnd);

            HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
            if (!hInst)
                hInst = GetModuleHandleA(nullptr);
            CreateHelloWindows(hInst);

            SetTimer(hwnd, ID_TIMER, 60000, nullptr);
            InvalidateRect(hwnd, nullptr, TRUE);
        }
        else if (wParam == ID_ANIM_TIMER && g_loading)
        {
            UpdateDrops(GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
            InvalidateRect(hwnd, nullptr, FALSE);
        }
        else if (wParam == ID_TIMER)
        {
            g_timerExpired = true;
            KillTimer(hwnd, ID_TIMER);
            TryOpenFlood(hwnd);
            if (!g_floodOpened)
            {
                MessageBoxA(hwnd,
                    "Таймер закончился!\nПосле 6 ошибок откроется спам.",
                    "Инфо", MB_OK | MB_ICONINFORMATION);
            }
        }
        return 0;
    }

    case WM_CTLCOLORSTATIC:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(0, 255, 70));
        SetBkColor(hdc, RGB(0, 0, 0));
        SetBkMode(hdc, TRANSPARENT);
        static HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
        return (LRESULT)black;
    }

    case WM_CTLCOLOREDIT:
    {
        HDC hdc = (HDC)wParam;
        SetTextColor(hdc, RGB(0, 255, 70));
        SetBkColor(hdc, RGB(10, 10, 10));
        static HBRUSH editBrush = CreateSolidBrush(RGB(10, 10, 10));
        return (LRESULT)editBrush;
    }

    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        HBRUSH black = CreateSolidBrush(RGB(0, 0, 0));
        FillRect(hdc, &rc, black);
        DeleteObject(black);

        if (g_loading)
        {
            SetBkMode(hdc, TRANSPARENT);

            HFONT font = CreateFontW(
                12, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");
            HFONT oldFont = (HFONT)SelectObject(hdc, font);

            const wchar_t* chars = L"01𰻞ﾊﾐﾋｰｳｼﾅﾓﾆｻﾜﾂｵﾘｱﾎﾃﾏｹﾒｴｶｷﾑﾕﾗｾﾈｽﾀﾇﾍ";
            int charCount = (int)wcslen(chars);

            for (const auto& d : g_drops)
            {
                for (int j = 0; j < d.length; ++j)
                {
                    int py = (int)d.y - j * 12;
                    if (py < -20 || py > rc.bottom) continue;

                    wchar_t c = chars[rand() % charCount];
                    wchar_t str[2] = { c, 0 };

                    if (j == 0)
                        SetTextColor(hdc, RGB(200, 255, 200));
                    else
                        SetTextColor(hdc, RGB(0, 140 + rand() % 90, 0));

                    TextOutW(hdc, d.x, py, str, 1);
                }
            }

            SelectObject(hdc, oldFont);
            DeleteObject(font);

            if (g_skullImage)
            {
                int imgW = (int)g_skullImage->GetWidth();
                int imgH = (int)g_skullImage->GetHeight();

                int maxH = rc.bottom * 58 / 100;
                int maxW = rc.right * 58 / 100;

                float scale = min((float)maxW / imgW, (float)maxH / imgH);
                int drawW = (int)(imgW * scale);
                int drawH = (int)(imgH * scale);

                int dx = (rc.right - drawW) / 2;
                int dy = (rc.bottom - drawH) / 2;

                Graphics graphics(hdc);
                graphics.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                graphics.DrawImage(g_skullImage, dx, dy, drawW, drawH);
            }
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_ERASEBKGND:
        return 1;

    case WM_COMMAND:
    {
        if (g_loading)
            return 0;

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
                TryOpenFlood(hwnd);

                if (!g_floodOpened)
                {
                    char buf[128];
                    wsprintfA(buf, "Неверный пароль!\nПопытка: %d / 6", g_wrongAttempts);
                    MessageBoxA(hwnd, buf, "Ошибка", MB_OK | MB_ICONERROR);
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
        KillTimer(hwnd, ID_LOADING_TIMER);
        KillTimer(hwnd, ID_ANIM_TIMER);
        if (g_skullImage)
        {
            delete g_skullImage;
            g_skullImage = nullptr;
        }
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&g_gdiplusToken, &gdiplusStartupInput, nullptr);

    const int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSA mainClass = {};
    mainClass.lpfnWndProc   = MainWindowProc;
    mainClass.hInstance     = hInstance;
    mainClass.lpszClassName = MAIN_CLASS;
    mainClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    mainClass.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    RegisterClassA(&mainClass);

    WNDCLASSA helloClass = {};
    helloClass.lpfnWndProc   = HelloWindowProc;
    helloClass.hInstance     = hInstance;
    helloClass.lpszClassName = HELLO_CLASS;
    helloClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    helloClass.hCursor       = LoadCursorA(nullptr, IDC_ARROW);
    RegisterClassA(&helloClass);

    WNDCLASSW floodClass = {};
    floodClass.lpfnWndProc   = FloodWindowProc;
    floodClass.hInstance     = hInstance;
    floodClass.lpszClassName = FLOOD_CLASS;
    floodClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    floodClass.hCursor       = LoadCursorW(nullptr, (LPCWSTR)IDC_ARROW);
    RegisterClassW(&floodClass);

    g_mainWindow = CreateWindowExA(
        0, MAIN_CLASS, "Password Demo",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr);

    if (!g_mainWindow)
    {
        GdiplusShutdown(g_gdiplusToken);
        return 0;
    }

    MSG msg = {};
    while (GetMessageA(&msg, nullptr, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }

    GdiplusShutdown(g_gdiplusToken);
    return 0;
}
