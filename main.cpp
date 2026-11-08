#define UNICODE
#define _UNICODE
#define NOMINMAX

#include <windows.h>
#include <windowsx.h>
#include <gdiplus.h>

#include <string>
#include <vector>
#include <fstream>
#include <algorithm>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

using namespace Gdiplus;
using namespace std;

// =====================================================
// FAKESTEAM PRO 3.0
// Educational Steam / CS2 interface simulation
// =====================================================

const int WIN_W = 1250;
const int WIN_H = 800;

enum Page
{
    HOME,
    LIBRARY,
    INVENTORY,
    EQUIPMENT,
    COMMUNITY,
    PROFILE,
    SETTINGS
};

struct Item
{
    wstring name;
    wstring weapon;
    wstring rarity;

    int price;
    int slot;
};

vector<Item> inventory =
{
    {L"Redline", L"AK-47", L"Запрещённое", 2500, 0},
    {L"Asiimov", L"AWP", L"Тайное", 8500, 0},
    {L"Printstream", L"M4A1-S", L"Тайное", 12000, 0},
    {L"Cortex", L"USP-S", L"Засекреченное", 1800, 1},
    {L"Blaze", L"Desert Eagle", L"Запрещённое", 6500, 1},
    {L"Gamma Doppler", L"Нож", L"Тайное", 45000, 2},
    {L"Sport Gloves", L"Перчатки", L"Тайное", 32000, 3},
    {L"Smoke", L"Граната", L"Армейское", 300, 4},
    {L"Dragon Lore", L"AWP", L"Тайное", 150000, 0}
};

Page currentPage = HOME;

int selectedItem = -1;
int selectedTeam = 0;

int equipped[2][5] =
{
    {-1, -1, -1, -1, -1},
    {-1, -1, -1, -1, -1}
};

wstring searchText;

bool searchActive = false;
bool inspectMode = false;

bool gameRunning = false;
int gameTimer = 0;

int sortMode = 0;

HWND hwnd = nullptr;

ULONG_PTR gdiplusToken;

// =====================================================
// COLORS
// =====================================================

Color BG(255, 14, 17, 24);
Color SIDEBAR(255, 19, 24, 33);
Color PANEL(255, 25, 32, 43);
Color PANEL2(255, 34, 43, 56);

Color BLUE(255, 50, 125, 190);
Color GREEN(255, 100, 180, 110);

Color WHITE(255, 240, 243, 248);
Color GRAY(255, 145, 157, 173);
Color RED(255, 190, 75, 75);

// =====================================================
// FONTS
// =====================================================

Font fontSmall(
    L"Segoe UI", 12,
    FontStyleRegular, UnitPixel
);

Font fontNormal(
    L"Segoe UI", 15,
    FontStyleRegular, UnitPixel
);

Font fontMedium(
    L"Segoe UI", 19,
    FontStyleBold, UnitPixel
);

Font fontTitle(
    L"Segoe UI", 29,
    FontStyleBold, UnitPixel
);

Font fontBig(
    L"Segoe UI", 38,
    FontStyleBold, UnitPixel
);

// =====================================================
// DRAWING
// =====================================================

void FillRect(
    Graphics& g,
    int x, int y,
    int w, int h,
    Color color
)
{
    SolidBrush brush(color);

    g.FillRectangle(
        &brush,
        x, y, w, h
    );
}

void Rounded(
    Graphics& g,
    int x, int y,
    int w, int h,
    int radius,
    Color color
)
{
    GraphicsPath path;

    path.AddArc(
        x, y, radius * 2,
        radius * 2, 180, 90
    );

    path.AddArc(
        x + w - radius * 2,
        y, radius * 2,
        radius * 2, 270, 90
    );

    path.AddArc(
        x + w - radius * 2,
        y + h - radius * 2,
        radius * 2,
        radius * 2, 0, 90
    );

    path.AddArc(
        x, y + h - radius * 2,
        radius * 2,
        radius * 2, 90, 90
    );

    path.CloseFigure();

    SolidBrush brush(color);

    g.FillPath(&brush, &path);
}

void DrawLabel(
    Graphics& g,
    int x, int y,
    const wstring& text,
    Color color,
    Font& font
)
{
    SolidBrush brush(color);

    g.DrawString(
        text.c_str(),
        -1,
        &font,
        PointF((REAL)x, (REAL)y),
        &brush
    );
}

void DrawButton(
    Graphics& g,
    int x, int y,
    int w, int h,
    const wstring& text,
    Color color
)
{
    Rounded(g, x, y, w, h, 7, color);

    DrawLabel(
        g,
        x + 14,
        y + 9,
        text,
        WHITE,
        fontNormal
    );
}

void DrawHeader(
    Graphics& g,
    const wstring& title
)
{
    DrawLabel(
        g, 265, 35,
        title,
        WHITE,
        fontTitle
    );

    FillRect(
        g, 265, 85,
        930, 1,
        Color(255, 55, 65, 80)
    );
}

// =====================================================
// SAVE / LOAD
// =====================================================

void SaveData()
{
    ofstream file("fakesteam_save.dat", ios::binary);

    if (!file)
        return;

    file.write(
        (char*)equipped,
        sizeof(equipped)
    );
}

void LoadData()
{
    ifstream file("fakesteam_save.dat", ios::binary);

    if (!file)
        return;

    int temp[2][5];

    file.read(
        (char*)temp,
        sizeof(temp)
    );

    if (!file)
        return;

    for (int t = 0; t < 2; t++)
    {
        for (int s = 0; s < 5; s++)
        {
            int value = temp[t][s];

            if (value >= -1 &&
                value < (int)inventory.size())
            {
                equipped[t][s] = value;
            }
        }
    }
}

// =====================================================
// INVENTORY SEARCH
// =====================================================

wstring Lower(wstring text)
{
    if (!text.empty())
    {
        CharLowerBuffW(
            &text[0],
            (DWORD)text.size()
        );
    }

    return text;
}

vector<int> GetVisibleItems()
{
    vector<int> result;

    for (int i = 0; i < (int)inventory.size(); i++)
    {
        const Item& item = inventory[i];

        wstring query = Lower(searchText);

        if (!query.empty())
        {
            if (
                Lower(item.name).find(query) == wstring::npos &&
                Lower(item.weapon).find(query) == wstring::npos &&
                Lower(item.rarity).find(query) == wstring::npos
            )
            {
                continue;
            }
        }

        result.push_back(i);
    }

    if (sortMode == 1)
    {
        sort(
            result.begin(),
            result.end(),
            [](int a, int b)
            {
                return inventory[a].name <
                       inventory[b].name;
            }
        );
    }

    if (sortMode == 2)
    {
        sort(
            result.begin(),
            result.end(),
            [](int a, int b)
            {
                return inventory[a].price >
                       inventory[b].price;
            }
        );
    }

    return result;
}

// =====================================================
// SIDEBAR
// =====================================================

void DrawSidebar(Graphics& g)
{
    FillRect(
        g, 0, 0,
        230, WIN_H,
        SIDEBAR
    );

    DrawLabel(
        g, 25, 27,
        L"STEAM",
        WHITE,
        fontTitle
    );

    DrawLabel(
        g, 25, 66,
        L"FAKESTEAM PRO 3.0",
        GRAY,
        fontSmall
    );

    struct Navigation
    {
        const wchar_t* text;
        Page page;
    };

    Navigation nav[] =
    {
        {L"Главная", HOME},
        {L"Библиотека", LIBRARY},
        {L"Инвентарь CS2", INVENTORY},
        {L"Снаряжение", EQUIPMENT},
        {L"Сообщество", COMMUNITY},
        {L"Профиль", PROFILE},
        {L"Настройки", SETTINGS}
    };

    for (int i = 0; i < 7; i++)
    {
        int y = 145 + i * 58;

        if (currentPage == nav[i].page)
        {
            Rounded(
                g,
                12, y - 7,
                205, 43,
                7,
                Color(255, 42, 65, 91)
            );
        }

        DrawLabel(
            g,
            30, y,
            nav[i].text,
            currentPage == nav[i].page
                ? WHITE : GRAY,
            fontNormal
        );
    }

    FillRect(
        g, 20, 700,
        190, 1,
        Color(255, 55, 65, 80)
    );

    DrawLabel(
        g, 25, 720,
        L"Offline Mode",
        GREEN,
        fontSmall
    );
}

// =====================================================
// HOME PAGE
// =====================================================

void DrawHome(Graphics& g)
{
    DrawHeader(g, L"Главная");

    Rounded(
        g, 270, 125,
        900, 270,
        12,
        PANEL
    );

    DrawLabel(
        g, 305, 160,
        L"Добро пожаловать!",
        WHITE,
        fontTitle
    );

    DrawLabel(
        g, 305, 215,
        L"Твоя игровая библиотека",
        GRAY,
        fontNormal
    );

    Rounded(
        g, 305, 275,
        810, 95,
        8,
        PANEL2
    );

    DrawLabel(
        g, 330, 295,
        L"COUNTER-STRIKE 2",
        WHITE,
        fontMedium
    );

    DrawLabel(
        g, 330, 330,
        L"Тактический шутер",
        GRAY,
        fontSmall
    );

    DrawButton(
        g, 270, 440,
        210, 48,
        L"Открыть библиотеку",
        BLUE
    );

    DrawButton(
        g, 500, 440,
        210, 48,
        L"Инвентарь",
        GREEN
    );
}

// =====================================================
// LIBRARY
// =====================================================

void DrawLibrary(Graphics& g)
{
    DrawHeader(g, L"Библиотека");

    Rounded(
        g, 270, 125,
        900, 470,
        12,
        PANEL
    );

    DrawLabel(
        g, 310, 165,
        L"COUNTER-STRIKE 2",
        WHITE,
        fontTitle
    );

    DrawLabel(
        g, 310, 220,
        L"Ваша игровая библиотека",
        GRAY,
        fontNormal
    );

    Rounded(
        g, 310, 280,
        820, 170,
        10,
        PANEL2
    );

    DrawLabel(
        g, 345, 315,
        L"COUNTER-STRIKE 2",
        WHITE,
        fontBig
    );

    DrawLabel(
        g, 345, 375,
        L"Статус: локальная демонстрация",
        GREEN,
        fontNormal
    );

    if (gameRunning)
    {
        DrawButton(
            g, 310, 500,
            230, 50,
            L"Запуск...",
            GREEN
        );

        DrawLabel(
            g, 560, 510,
            L"Имитация запуска игры",
            GRAY,
            fontNormal
        );
    }
    else
    {
        DrawButton(
            g, 310, 500,
            230, 50,
            L"ИГРАТЬ",
            GREEN
        );
    }
}

// =====================================================
// INVENTORY
// =====================================================

void DrawInventory(Graphics& g)
{
    DrawHeader(g, L"Инвентарь CS2");

    if (inspectMode && selectedItem >= 0)
    {
        Item& item = inventory[selectedItem];

        DrawButton(
            g, 270, 110,
            110, 38,
            L"Назад",
            BLUE
        );

        Rounded(
            g, 270, 175,
            900, 420,
            12,
            PANEL
        );

        Rounded(
            g, 310, 220,
            330, 310,
            10,
            PANEL2
        );

        DrawLabel(
            g, 355, 345,
            item.weapon,
            WHITE,
            fontTitle
        );

        DrawLabel(
            g, 355, 395,
            item.name,
            GREEN,
            fontMedium
        );

        DrawLabel(
            g, 690, 245,
            L"Информация о предмете",
            WHITE,
            fontTitle
        );

        DrawLabel(
            g, 690, 320,
            L"Оружие:",
            GRAY,
            fontNormal
        );

        DrawLabel(
            g, 850, 320,
            item.weapon,
            WHITE,
            fontNormal
        );

        DrawLabel(
            g, 690, 365,
            L"Название:",
            GRAY,
            fontNormal
        );

        DrawLabel(
            g, 850, 365,
            item.name,
            WHITE,
            fontNormal
        );

        DrawLabel(
            g, 690, 410,
            L"Редкость:",
            GRAY,
            fontNormal
        );

        DrawLabel(
            g, 850, 410,
            item.rarity,
            GREEN,
            fontNormal
        );

        DrawLabel(
            g, 690, 455,
            L"Демо-стоимость:",
            GRAY,
            fontNormal
        );

        DrawLabel(
            g, 850, 455,
            to_wstring(item.price) + L" ₽",
            WHITE,
            fontNormal
        );

        DrawButton(
            g, 690, 510,
            190, 45,
            L"Экипировать",
            GREEN
        );

        return;
    }

    Rounded(
        g, 270, 110,
        480, 40,
        7,
        PANEL2
    );

    DrawLabel(
        g, 285, 120,
        searchText.empty()
            ? L"Поиск предметов..."
            : searchText,
        searchActive ? WHITE : GRAY,
        fontNormal
    );

    DrawButton(
        g, 770, 110,
        170, 40,
        sortMode == 0 ? L"Без сортировки" :
        sortMode == 1 ? L"По названию" :
                        L"По стоимости",
        BLUE
    );

    vector<int> visible = GetVisibleItems();

    DrawLabel(
        g, 960, 120,
        L"Предметов: " + to_wstring(visible.size()),
        GRAY,
        fontSmall
    );

    for (int v = 0; v < (int)visible.size(); v++)
    {
        int index = visible[v];

        int col = v % 3;
        int row = v / 3;

        int x = 270 + col * 305;
        int y = 180 + row * 185;

        Item& item = inventory[index];

        Rounded(
            g, x, y,
            285, 165,
            9,
            PANEL
        );

        Rounded(
            g, x + 15, y + 15,
            255, 75,
            7,
            PANEL2
        );

        DrawLabel(
            g, x + 28, y + 35,
            item.weapon,
            WHITE,
            fontMedium
        );

        DrawLabel(
            g, x + 20, y + 105,
            item.name,
            GREEN,
            fontNormal
        );

        DrawLabel(
            g, x + 20, y + 132,
            item.rarity,
            GRAY,
            fontSmall
        );
    }
}

// =====================================================
// EQUIPMENT
// =====================================================

void DrawEquipment(Graphics& g)
{
    DrawHeader(g, L"Снаряжение");

    DrawButton(
        g, 275, 115,
        150, 42,
        L"Террористы",
        selectedTeam == 0 ? GREEN : PANEL2
    );

    DrawButton(
        g, 440, 115,
        150, 42,
        L"Спецназ",
        selectedTeam == 1 ? GREEN : PANEL2
    );

    const wchar_t* slots[] =
    {
        L"Основное оружие",
        L"Пистолет",
        L"Нож",
        L"Перчатки",
        L"Гранаты"
    };

    for (int i = 0; i < 5; i++)
    {
        int y = 190 + i * 100;

        Rounded(
            g, 275, y,
            870, 80,
            8,
            PANEL
        );

        DrawLabel(
            g, 300, y + 12,
            slots[i],
            WHITE,
            fontMedium
        );

        int index = equipped[selectedTeam][i];

        wstring text = L"Не экипировано";

        if (index >= 0 &&
            index < (int)inventory.size())
        {
            text =
                inventory[index].weapon +
                L" | " +
                inventory[index].name;
        }

        DrawLabel(
            g, 700, y + 25,
            text,
            GREEN,
            fontNormal
        );
    }
}

// =====================================================
// OTHER PAGES
// =====================================================

void DrawOtherPage(
    Graphics& g,
    const wstring& title
)
{
    DrawHeader(g, title);

    Rounded(
        g, 275, 130,
        850, 300,
        12,
        PANEL
    );

    DrawLabel(
        g, 310, 180,
        title,
        WHITE,
        fontTitle
    );

    DrawLabel(
        g, 310, 245,
        L"Раздел находится в разработке.",
        GRAY,
        fontNormal
    );
}

// =====================================================
// MAIN RENDER
// =====================================================

void Render(Graphics& g)
{
    g.SetSmoothingMode(SmoothingModeAntiAlias);
    g.SetTextRenderingHint(TextRenderingHintAntiAliasGridFit);

    FillRect(
        g, 0, 0,
        WIN_W, WIN_H,
        BG
    );

    DrawSidebar(g);

    switch (currentPage)
    {
        case HOME:
            DrawHome(g);
            break;

        case LIBRARY:
            DrawLibrary(g);
            break;

        case INVENTORY:
            DrawInventory(g);
            break;

        case EQUIPMENT:
            DrawEquipment(g);
            break;

        case COMMUNITY:
            DrawOtherPage(g, L"Сообщество");
            break;

        case PROFILE:
            DrawOtherPage(g, L"Профиль");
            break;

        case SETTINGS:
            DrawOtherPage(g, L"Настройки");
            break;
    }
}

// =====================================================
// NAVIGATION
// =====================================================

void ChangePage(Page page)
{
    currentPage = page;

    inspectMode = false;
    searchActive = false;

    InvalidateRect(hwnd, nullptr, FALSE);
}

// =====================================================
// MOUSE
// =====================================================

void HandleClick(int x, int y)
{
    // SIDEBAR

    if (x < 230)
    {
        if (y >= 130 && y < 188)
            ChangePage(HOME);

        else if (y >= 188 && y < 246)
            ChangePage(LIBRARY);

        else if (y >= 246 && y < 304)
            ChangePage(INVENTORY);

        else if (y >= 304 && y < 362)
            ChangePage(EQUIPMENT);

        else if (y >= 362 && y < 420)
            ChangePage(COMMUNITY);

        else if (y >= 420 && y < 478)
            ChangePage(PROFILE);

        else if (y >= 478 && y < 536)
            ChangePage(SETTINGS);

        return;
    }

    // HOME

    if (currentPage == HOME)
    {
        if (y >= 440 && y <= 500)
        {
            if (x >= 270 && x <= 480)
                ChangePage(LIBRARY);

            else if (x >= 500 && x <= 710)
                ChangePage(INVENTORY);
        }

        return;
    }

    // LIBRARY

    if (currentPage == LIBRARY)
    {
        if (x >= 310 && x <= 540 &&
            y >= 500 && y <= 550)
        {
            gameRunning = true;
            gameTimer = 5;

            SetTimer(hwnd, 1, 1000, nullptr);

            InvalidateRect(hwnd, nullptr, FALSE);
        }

        return;
    }

    // INVENTORY

    if (currentPage == INVENTORY)
    {
        if (inspectMode)
        {
            if (x >= 270 && x <= 380 &&
                y >= 110 && y <= 148)
            {
                inspectMode = false;
            }

            if (x >= 690 && x <= 880 &&
                y >= 510 && y <= 555)
            {
                if (selectedItem >= 0)
                {
                    int slot = inventory[selectedItem].slot;

                    if (slot >= 0 && slot < 5)
                    {
                        equipped[selectedTeam][slot] =
                            selectedItem;

                        SaveData();
                    }
                }
            }

            InvalidateRect(hwnd, nullptr, FALSE);
            return;
        }

        if (x >= 270 && x <= 750 &&
            y >= 110 && y <= 150)
        {
            searchActive = true;
            return;
        }

        searchActive = false;

        if (x >= 770 && x <= 940 &&
            y >= 110 && y <= 150)
        {
            sortMode = (sortMode + 1) % 3;
        }

        vector<int> visible = GetVisibleItems();

        for (int v = 0; v < (int)visible.size(); v++)
        {
            int col = v % 3;
            int row = v / 3;

            int cx = 270 + col * 305;
            int cy = 180 + row * 185;

            if (x >= cx && x <= cx + 285 &&
                y >= cy && y <= cy + 165)
            {
                selectedItem = visible[v];
                inspectMode = true;
                break;
            }
        }

        InvalidateRect(hwnd, nullptr, FALSE);
        return;
    }

    // EQUIPMENT

    if (currentPage == EQUIPMENT)
    {
        if (y >= 115 && y <= 157)
        {
            if (x >= 275 && x <= 425)
                selectedTeam = 0;

            if (x >= 440 && x <= 590)
                selectedTeam = 1;
        }

        InvalidateRect(hwnd, nullptr, FALSE);
    }
}

// =====================================================
// WINDOW PROCEDURE
// =====================================================

LRESULT CALLBACK WindowProc(
    HWND window,
    UINT msg,
    WPARAM wp,
    LPARAM lp
)
{
    switch (msg)
    {
        case WM_LBUTTONDOWN:
        {
            SetFocus(window);

            HandleClick(
                GET_X_LPARAM(lp),
                GET_Y_LPARAM(lp)
            );

            return 0;
        }

        case WM_CHAR:
        {
            if (currentPage == INVENTORY &&
                searchActive &&
                !inspectMode)
            {
                wchar_t ch = (wchar_t)wp;

                if (ch == L'\b')
                {
                    if (!searchText.empty())
                        searchText.pop_back();
                }
                else if (ch >= L' ' &&
                         searchText.size() < 40)
                {
                    searchText.push_back(ch);
                }

                InvalidateRect(
                    window,
                    nullptr,
                    FALSE
                );

                return 0;
            }

            break;
        }

        case WM_TIMER:
        {
            if (wp == 1 && gameRunning)
            {
                gameTimer--;

                if (gameTimer <= 0)
                {
                    gameRunning = false;
                    KillTimer(window, 1);
                }

                InvalidateRect(
                    window,
                    nullptr,
                    FALSE
                );

                return 0;
            }

            break;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            HDC hdc = BeginPaint(window, &ps);

            Graphics graphics(hdc);

            Render(graphics);

            EndPaint(window, &ps);

            return 0;
        }

        case WM_DESTROY:
        {
            SaveData();

            PostQuitMessage(0);

            return 0;
        }
    }

    return DefWindowProc(
        window,
        msg,
        wp,
        lp
    );
}

// =====================================================
// ENTRY POINT
// =====================================================

int WINAPI WinMain(
    HINSTANCE instance,
    HINSTANCE,
    LPSTR,
    int
)
{
    GdiplusStartupInput gdiplusStartupInput;

    GdiplusStartup(
        &gdiplusToken,
        &gdiplusStartupInput,
        nullptr
    );

    LoadData();

    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = instance;
    wc.lpszClassName = L"FakeSteamProWindow";
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;

    RegisterClassW(&wc);

    hwnd = CreateWindowExW(
        0,
        wc.lpszClassName,
        L"FakeSteamPro 3.0",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        WIN_W,
        WIN_H,
        nullptr,
        nullptr,
        instance,
        nullptr
    );

    if (!hwnd)
    {
        GdiplusShutdown(gdiplusToken);
        return 1;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    MSG msg = {};

    while (GetMessage(&msg, nullptr, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    GdiplusShutdown(gdiplusToken);

    return 0;
}
