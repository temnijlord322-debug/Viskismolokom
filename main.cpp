#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace std;

// ==========================================
// DEEBSOFTING 1.0
// ==========================================

const int WIDTH = 1050;
const int HEIGHT = 680;

const COLORREF BG       = RGB(12, 12, 15);
const COLORREF SIDEBAR  = RGB(17, 17, 21);
const COLORREF PANEL    = RGB(23, 23, 28);
const COLORREF PANEL2   = RGB(29, 29, 35);
const COLORREF RED      = RGB(220, 35, 48);
const COLORREF RED2     = RGB(150, 20, 32);
const COLORREF WHITE    = RGB(240, 240, 245);
const COLORREF GRAY     = RGB(135, 135, 145);
const COLORREF GREEN    = RGB(65, 190, 110);

HWND hwnd;

HFONT fontNormal;
HFONT fontSmall;
HFONT fontTitle;
HFONT fontBig;

int currentPage = 0;

bool menuVisible = true;

struct Setting
{
    wstring name;
    bool enabled;
    int value;
    int minimum;
    int maximum;
    bool slider;
};

vector<Setting> settings[7];

// ==========================================
// DRAW FUNCTIONS
// ==========================================

void Fill(HDC dc, int x, int y, int w, int h, COLORREF color)
{
    HBRUSH brush = CreateSolidBrush(color);

    RECT r = { x, y, x + w, y + h };

    FillRect(dc, &r, brush);

    DeleteObject(brush);
}

void Text(
    HDC dc,
    int x,
    int y,
    const wstring& str,
    COLORREF color,
    HFONT font = nullptr
)
{
    SetTextColor(dc, color);
    SetBkMode(dc, TRANSPARENT);

    HFONT old = nullptr;

    if (font)
        old = (HFONT)SelectObject(dc, font);

    TextOutW(
        dc,
        x,
        y,
        str.c_str(),
        (int)str.size()
    );

    if (old)
        SelectObject(dc, old);
}

void Rounded(
    HDC dc,
    int x,
    int y,
    int w,
    int h,
    COLORREF color
)
{
    HBRUSH brush = CreateSolidBrush(color);
    HPEN pen = CreatePen(PS_SOLID, 1, color);

    HBRUSH oldBrush =
        (HBRUSH)SelectObject(dc, brush);

    HPEN oldPen =
        (HPEN)SelectObject(dc, pen);

    RoundRect(
        dc,
        x, y,
        x + w, y + h,
        9, 9
    );

    SelectObject(dc, oldBrush);
    SelectObject(dc, oldPen);

    DeleteObject(brush);
    DeleteObject(pen);
}

// ==========================================
// SETTINGS
// ==========================================

void AddToggle(
    int page,
    const wstring& name,
    bool enabled = false
)
{
    Setting s;

    s.name = name;
    s.enabled = enabled;
    s.value = 0;
    s.minimum = 0;
    s.maximum = 0;
    s.slider = false;

    settings[page].push_back(s);
}

void AddSlider(
    int page,
    const wstring& name,
    int value,
    int minimum,
    int maximum
)
{
    Setting s;

    s.name = name;
    s.enabled = false;
    s.value = value;
    s.minimum = minimum;
    s.maximum = maximum;
    s.slider = true;

    settings[page].push_back(s);
}

void Initialize()
{
    // VISUALS

    AddToggle(1, L"ESP Preview", true);
    AddToggle(1, L"Box ESP", true);
    AddToggle(1, L"Health Bar", true);
    AddToggle(1, L"Player Names", true);
    AddToggle(1, L"Distance", false);
    AddToggle(1, L"Skeleton", false);
    AddToggle(1, L"Tracers", false);

    AddSlider(1, L"Box Thickness", 2, 1, 5);
    AddSlider(1, L"ESP Opacity", 85, 0, 100);

    // AIMBOT

    AddToggle(2, L"Aimbot Preview", true);
    AddToggle(2, L"Head Priority", true);
    AddToggle(2, L"No Scope Preview", false);
    AddToggle(2, L"Air Shot Preview", false);

    AddSlider(2, L"FOV", 35, 1, 180);
    AddSlider(2, L"Smooth", 5, 1, 20);
    AddSlider(2, L"Distance", 100, 1, 500);

    // ANTI AIM

    AddToggle(3, L"Anti-Aim Preview", false);
    AddToggle(3, L"Fake Angles Preview", false);
    AddToggle(3, L"Rotation Preview", false);

    AddSlider(3, L"Rotation Speed", 5, 1, 10);
    AddSlider(3, L"Rotation Angle", 90, 0, 360);

    // MOVEMENT

    AddToggle(4, L"Bunny Hop Preview", false);
    AddToggle(4, L"Auto-Strafe Preview", false);
    AddToggle(4, L"Auto-Stop Preview", false);

    AddSlider(4, L"Bunny Hop Intensity", 5, 1, 10);

    // MISC

    AddToggle(5, L"Third Person Preview", false);
    AddToggle(5, L"Triggerbot Preview", false);
    AddToggle(5, L"Watermark", true);
    AddToggle(5, L"Show FPS", true);

    // CONFIGS

    AddToggle(6, L"Auto Save", true);
}

// ==========================================
// SAVE / LOAD
// ==========================================

void SaveConfig()
{
    ofstream file("deebsofting.ini");

    if (!file.is_open())
        return;

    for (int p = 0; p < 7; p++)
    {
        for (int i = 0; i < (int)settings[p].size(); i++)
        {
            file
                << p << " "
                << i << " "
                << settings[p][i].enabled << " "
                << settings[p][i].value
                << "\n";
        }
    }
}

void LoadConfig()
{
    ifstream file("deebsofting.ini");

    if (!file.is_open())
        return;

    int page, index, enabled, value;

    while (file >> page >> index >> enabled >> value)
    {
        if (page >= 0 && page < 7)
        {
            if (index >= 0 &&
                index < (int)settings[page].size())
            {
                settings[page][index].enabled =
                    enabled != 0;

                settings[page][index].value =
                    value;
            }
        }
    }
}

// ==========================================
// NAVIGATION
// ==========================================

const wchar_t* pages[] =
{
    L"Dashboard",
    L"Visuals",
    L"Aimbot",
    L"Anti-Aim",
    L"Movement",
    L"Misc",
    L"Configs"
};

void DrawSidebar(HDC dc)
{
    Fill(dc, 0, 0, 230, HEIGHT, SIDEBAR);

    // LOGO

    Rounded(dc, 20, 20, 42, 42, RED);

    Text(
        dc,
        30,
        30,
        L"D",
        WHITE,
        fontBig
    );

    Text(
        dc,
        72,
        22,
        L"DEEBSOFTING",
        WHITE,
        fontTitle
    );

    Text(
        dc,
        72,
        48,
        L"PREMIUM INTERFACE",
        GRAY,
        fontSmall
    );

    Fill(dc, 18, 85, 194, 1, RGB(45, 45, 52));

    // NAVIGATION

    for (int i = 0; i < 7; i++)
    {
        int y = 110 + i * 58;

        if (currentPage == i)
        {
            Rounded(
                dc,
                13,
                y,
                203,
                45,
                RED2
            );

            Fill(
                dc,
                13,
                y + 8,
                3,
                29,
                RED
            );
        }

        Text(
            dc,
            32,
            y + 13,
            pages[i],
            currentPage == i ? WHITE : GRAY,
            fontNormal
        );
    }

    Fill(dc, 18, 625, 194, 1, RGB(45, 45, 52));

    Text(
        dc,
        25,
        642,
        L"DEEBSOFTING 1.0",
        GRAY,
        fontSmall
    );
}

// ==========================================
// DASHBOARD
// ==========================================

void DrawDashboard(HDC dc)
{
    Text(
        dc,
        265,
        35,
        L"Dashboard",
        WHITE,
        fontTitle
    );

    Text(
        dc,
        265,
        75,
        L"Welcome to DEEBSOFTING",
        GRAY,
        fontNormal
    );

    // STATUS

    Rounded(dc, 265, 125, 730, 105, PANEL);

    Fill(dc, 265, 125, 4, 105, RED);

    Text(
        dc,
        290,
        150,
        L"APPLICATION STATUS",
        GRAY,
        fontSmall
    );

    Text(
        dc,
        290,
        180,
        L"Interface is running",
        GREEN,
        fontTitle
    );

    // CARDS

    const wchar_t* cardNames[] =
    {
        L"VISUALS",
        L"AIMBOT",
        L"ANTI-AIM"
    };

    for (int i = 0; i < 3; i++)
    {
        int x = 265 + i * 245;

        Rounded(
            dc,
            x,
            260,
            225,
            130,
            PANEL
        );

        Text(
            dc,
            x + 20,
            280,
            cardNames[i],
            GRAY,
            fontSmall
        );

        Text(
            dc,
            x + 20,
            320,
            L"CONFIGURATION",
            WHITE,
            fontTitle
        );

        Text(
            dc,
            x + 20,
            355,
            L"Open section",
            RED,
            fontSmall
        );
    }

    Rounded(dc, 265, 420, 730, 155, PANEL);

    Text(
        dc,
        290,
        445,
        L"INFORMATION",
        WHITE,
        fontTitle
    );

    Text(
        dc,
        290,
        490,
        L"Standalone interface and local simulation.",
        GRAY
    );

    Text(
        dc,
        290,
        520,
        L"No connection to the CS2 process.",
        GRAY
    );
}

// ==========================================
// SETTINGS DRAWING
// ==========================================

void DrawSettings(HDC dc)
{
    Text(
        dc,
        265,
        35,
        pages[currentPage],
        WHITE,
        fontTitle
    );

    Text(
        dc,
        265,
        75,
        L"Customize your configuration",
        GRAY
    );

    auto& list = settings[currentPage];

    int y = 125;

    for (int i = 0; i < (int)list.size(); i++)
    {
        Setting& s = list[i];

        Rounded(
            dc,
            265,
            y,
            730,
            55,
            PANEL
        );

        Text(
            dc,
            290,
            y + 18,
            s.name,
            WHITE,
            fontNormal
        );

        if (!s.slider)
        {
            COLORREF color =
                s.enabled ? RED : RGB(65, 65, 72);

            Rounded(
                dc,
                925,
                y + 15,
                48,
                25,
                color
            );

            int circleX =
                s.enabled ? 951 : 929;

            HBRUSH brush =
                CreateSolidBrush(WHITE);

            HBRUSH old =
                (HBRUSH)SelectObject(dc, brush);

            HPEN pen = CreatePen(
                PS_NULL,
                0,
                WHITE
            );

            HPEN oldPen =
                (HPEN)SelectObject(dc, pen);

            Ellipse(
                dc,
                circleX,
                y + 18,
                circleX + 19,
                y + 37
            );

            SelectObject(dc, old);
            SelectObject(dc, oldPen);

            DeleteObject(brush);
            DeleteObject(pen);
        }
        else
        {
            Text(
                dc,
                900,
                y + 17,
                to_wstring(s.value),
                RED,
                fontNormal
            );

            Fill(
                dc,
                750,
                y + 26,
                130,
                4,
                RGB(65, 65, 72)
            );

            int pos =
                750 +
                (s.value - s.minimum) * 130 /
                max(1, s.maximum - s.minimum);

            Fill(
                dc,
                750,
                y + 26,
                pos - 750,
                4,
                RED
            );

            Rounded(
                dc,
                pos - 6,
                y + 19,
                12,
                18,
                RED
            );
        }

        y += 68;
    }

    if (currentPage == 6)
    {
        Text(
            dc,
            290,
            610,
            L"Configuration is saved in deebsofting.ini",
            GRAY,
            fontSmall
        );
    }
}

// ==========================================
// PAINT
// ==========================================

void DrawInterface(HDC dc)
{
    Fill(dc, 0, 0, WIDTH, HEIGHT, BG);

    DrawSidebar(dc);

    if (currentPage == 0)
        DrawDashboard(dc);
    else
        DrawSettings(dc);

    // TOP RIGHT STATUS

    Rounded(dc, 850, 20, 145, 32, PANEL2);

    Text(
        dc,
        870,
        28,
        L"●  ONLINE",
        GREEN,
        fontSmall
    );
}

// ==========================================
// MOUSE
// ==========================================

void HandleClick(int x, int y)
{
    // SIDEBAR

    if (x < 230)
    {
        if (y >= 110 && y < 110 + 7 * 58)
        {
            currentPage =
                (y - 110) / 58;

            InvalidateRect(hwnd, nullptr, TRUE);
        }

        return;
    }

    if (currentPage == 0)
        return;

    auto& list = settings[currentPage];

    int row = (y - 125) / 68;

    if (row < 0 || row >= (int)list.size())
        return;

    Setting& s = list[row];

    if (!s.slider)
    {
        s.enabled = !s.enabled;
    }
    else
    {
        int newValue =
            s.minimum +
            (x - 750) * (s.maximum - s.minimum) / 130;

        s.value = max(
            s.minimum,
            min(s.maximum, newValue)
        );
    }

    SaveConfig();

    InvalidateRect(hwnd, nullptr, TRUE);
}

// ==========================================
// WINDOW PROCEDURE
// ==========================================

LRESULT CALLBACK WindowProc(
    HWND window,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;

        HDC dc = BeginPaint(window, &ps);

        HDC memory = CreateCompatibleDC(dc);

        HBITMAP bitmap = CreateCompatibleBitmap(
            dc,
            WIDTH,
            HEIGHT
        );

        HBITMAP old =
            (HBITMAP)SelectObject(memory, bitmap);

        DrawInterface(memory);

        BitBlt(
            dc,
            0,
            0,
            WIDTH,
            HEIGHT,
            memory,
            0,
            0,
            SRCCOPY
        );

        SelectObject(memory, old);

        DeleteObject(bitmap);
        DeleteDC(memory);

        EndPaint(window, &ps);

        return 0;
    }

    case WM_LBUTTONDOWN:
    {
        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        HandleClick(x, y);

        return 0;
    }

    case WM_HOTKEY:
    {
        if (wParam == 1)
        {
            menuVisible = !menuVisible;

            ShowWindow(
                window,
                menuVisible ? SW_SHOW : SW_HIDE
            );
        }

        if (wParam == 2)
        {
            ShowWindow(window, SW_HIDE);
            menuVisible = false;
        }

        if (wParam == 3)
        {
            DestroyWindow(window);
        }

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(
        window,
        msg,
        wParam,
        lParam
    );
}

// ==========================================
// MAIN
// ==========================================

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPSTR,
    int
)
{
    Initialize();
    LoadConfig();

    fontNormal = CreateFontW(
        17, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Segoe UI"
    );

    fontSmall = CreateFontW(
        12, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Segoe UI"
    );

    fontTitle = CreateFontW(
        24, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Segoe UI"
    );

    fontBig = CreateFontW(
        32, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Segoe UI"
    );

    WNDCLASSW wc{};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"DEEBSOFTING_WINDOW";
    wc.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(BG);

    RegisterClassW(&wc);

    hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"DEEBSOFTING",
        WS_OVERLAPPEDWINDOW & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WIDTH,
        HEIGHT,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!hwnd)
        return 1;

    RegisterHotKey(hwnd, 1, 0, VK_INSERT);
    RegisterHotKey(hwnd, 2, 0, VK_DELETE);
    RegisterHotKey(hwnd, 3, 0, VK_END);

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg{};

    while (GetMessageW(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    UnregisterHotKey(hwnd, 1);
    UnregisterHotKey(hwnd, 2);
    UnregisterHotKey(hwnd, 3);

    DeleteObject(fontNormal);
    DeleteObject(fontSmall);
    DeleteObject(fontTitle);
    DeleteObject(fontBig);

    return 0;
}
