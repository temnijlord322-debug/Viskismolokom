#include <windows.h>
#include <string>
#include <vector>

#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "user32.lib")

enum Page {
    STORE,
    LIBRARY,
    INVENTORY,
    EQUIPMENT,
    COMMUNITY,
    PROFILE,
    SETTINGS
};

Page currentPage = LIBRARY;
std::wstring statusText = L"Добро пожаловать в Steam";

HFONT fontMain;
HFONT fontTitle;

COLORREF BG = RGB(23, 26, 33);
COLORREF PANEL = RGB(27, 40, 56);
COLORREF BLUE = RGB(31, 105, 160);
COLORREF GREEN = RGB(120, 190, 45);
COLORREF WHITE = RGB(230, 235, 240);
COLORREF GRAY = RGB(160, 170, 180);

void Text(HDC hdc, int x, int y, const std::wstring& text,
          COLORREF color, HFONT font = nullptr) {

    SetTextColor(hdc, color);
    SetBkMode(hdc, TRANSPARENT);

    HFONT old = nullptr;

    if (font)
        old = (HFONT)SelectObject(hdc, font);

    TextOutW(hdc, x, y, text.c_str(), (int)text.length());

    if (old)
        SelectObject(hdc, old);
}

void Rect(HDC hdc, int x, int y, int w, int h, COLORREF color) {
    HBRUSH brush = CreateSolidBrush(color);

    RECT r = { x, y, x + w, y + h };

    FillRect(hdc, &r, brush);

    DeleteObject(brush);
}

void Button(HDC hdc, int x, int y, int w, int h,
            const std::wstring& title, COLORREF color) {

    HBRUSH brush = CreateSolidBrush(color);

    HPEN pen = CreatePen(PS_SOLID, 1, color);

    HGDIOBJ oldBrush = SelectObject(hdc, brush);
    HGDIOBJ oldPen = SelectObject(hdc, pen);

    RoundRect(hdc, x, y, x + w, y + h, 8, 8);

    SelectObject(hdc, oldBrush);
    SelectObject(hdc, oldPen);

    DeleteObject(brush);
    DeleteObject(pen);

    SetTextColor(hdc, WHITE);
    SetBkMode(hdc, TRANSPARENT);

    RECT r = { x, y, x + w, y + h };

    DrawTextW(
        hdc,
        title.c_str(),
        -1,
        &r,
        DT_CENTER | DT_VCENTER | DT_SINGLELINE
    );
}

void DrawSidebar(HDC hdc) {

    Rect(hdc, 0, 0, 220, 760, RGB(20, 29, 40));

    Text(hdc, 25, 25, L"STEAM", RGB(80, 170, 220), fontTitle);

    Text(hdc, 25, 100, L"ГЛАВНОЕ", GRAY);

    struct MenuItem {
        const wchar_t* name;
        Page page;
    };

    MenuItem items[] = {
        { L"Магазин", STORE },
        { L"Библиотека", LIBRARY },
        { L"Сообщество", COMMUNITY },
        { L"Профиль", PROFILE },
        { L"Инвентарь", INVENTORY },
        { L"Снаряжение", EQUIPMENT },
        { L"Настройки", SETTINGS }
    };

    int y = 135;

    for (auto& item : items) {

        COLORREF color =
            currentPage == item.page
            ? RGB(40, 85, 120)
            : RGB(20, 29, 40);

        Rect(hdc, 10, y - 8, 200, 38, color);

        Text(hdc, 28, y, item.name, WHITE);

        y += 48;
    }
}

void DrawLibrary(HDC hdc) {

    Text(hdc, 260, 55, L"БИБЛИОТЕКА", GRAY);

    Text(hdc, 260, 110,
         L"Counter-Strike 2",
         WHITE, fontTitle);

    Rect(hdc, 260, 160, 880, 280, RGB(35, 65, 90));

    Text(hdc, 300, 210,
         L"COUNTER-STRIKE 2",
         WHITE, fontTitle);

    Text(hdc, 300, 270,
         L"Тактический шутер",
         RGB(190, 205, 220));

    Text(hdc, 300, 310,
         L"Ваша библиотека Steam",
         RGB(190, 205, 220));

    Button(hdc, 300, 365, 180, 50,
           L"ИГРАТЬ", GREEN);

    Text(hdc, 260, 490,
         L"ПОСЛЕДНЯЯ АКТИВНОСТЬ",
         GRAY);

    Text(hdc, 260, 530,
         L"Counter-Strike 2",
         WHITE);

    Text(hdc, 260, 565,
         L"Игра добавлена в библиотеку",
         GRAY);
}

void DrawInventory(HDC hdc) {

    Text(hdc, 260, 55, L"ИНВЕНТАРЬ CS2", WHITE, fontTitle);

    Text(hdc, 260, 105,
         L"Ваши предметы",
         GRAY);

    struct Item {
        const wchar_t* name;
        const wchar_t* rarity;
        COLORREF color;
    };

    Item items[] = {
        { L"AK-47 | Redline", L"Запрещённое", RGB(180, 55, 55) },
        { L"AWP | Asiimov", L"Тайное", RGB(230, 120, 45) },
        { L"USP-S | Cortex", L"Засекреченное", RGB(170, 90, 160) },
        { L"Desert Eagle", L"Армейское", RGB(90, 130, 170) },
        { L"★ Нож", L"Тайное", RGB(190, 155, 50) },
        { L"Перчатки", L"Тайное", RGB(160, 100, 70) }
    };

    int x = 260;
    int y = 150;

    for (int i = 0; i < 6; i++) {

        Rect(hdc, x, y, 240, 155, PANEL);

        Rect(hdc, x + 10, y + 10, 220, 85,
             items[i].color);

        Text(hdc, x + 15, y + 105,
             items[i].name, WHITE);

        Text(hdc, x + 15, y + 130,
             items[i].rarity, GRAY);

        x += 270;

        if (x > 900) {
            x = 260;
            y += 180;
        }
    }
}

void DrawEquipment(HDC hdc) {

    Text(hdc, 260, 55,
         L"СНАРЯЖЕНИЕ",
         WHITE, fontTitle);

    Button(hdc, 260, 120, 180, 45,
           L"ТЕРРОРИСТЫ", RGB(145, 105, 45));

    Button(hdc, 460, 120, 180, 45,
           L"СПЕЦНАЗ", BLUE);

    Text(hdc, 260, 220,
         L"Основное оружие",
         WHITE);

    Text(hdc, 260, 275,
         L"AK-47",
         GRAY);

    Text(hdc, 260, 330,
         L"Вторичное оружие",
         WHITE);

    Text(hdc, 260, 385,
         L"Glock-18",
         GRAY);

    Text(hdc, 260, 440,
         L"Нож",
         WHITE);

    Text(hdc, 260, 495,
         L"Перчатки",
         WHITE);
}

void DrawPage(HDC hdc) {

    switch (currentPage) {

    case LIBRARY:
        DrawLibrary(hdc);
        break;

    case INVENTORY:
        DrawInventory(hdc);
        break;

    case EQUIPMENT:
        DrawEquipment(hdc);
        break;

    case STORE:
        Text(hdc, 260, 70,
             L"МАГАЗИН STEAM",
             WHITE, fontTitle);
        Text(hdc, 260, 130,
             L"Добро пожаловать в магазин",
             GRAY);
        break;

    case COMMUNITY:
        Text(hdc, 260, 70,
             L"СООБЩЕСТВО",
             WHITE, fontTitle);
        break;

    case PROFILE:
        Text(hdc, 260, 70,
             L"ВАШ ПРОФИЛЬ",
             WHITE, fontTitle);
        Text(hdc, 260, 130,
             L"Игрок Steam",
             GRAY);
        break;

    case SETTINGS:
        Text(hdc, 260, 70,
             L"НАСТРОЙКИ",
             WHITE, fontTitle);
        break;
    }

    Text(hdc, 260, 700, statusText, GREEN);
}

LRESULT CALLBACK WindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam
) {

    switch (msg) {

    case WM_PAINT: {

        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT r;
        GetClientRect(hwnd, &r);

        Rect(hdc, 0, 0, r.right, r.bottom, BG);

        DrawSidebar(hdc);
        DrawPage(hdc);

        EndPaint(hwnd, &ps);

        return 0;
    }

    case WM_LBUTTONDOWN: {

        int x = LOWORD(lParam);
        int y = HIWORD(lParam);

        if (x < 220) {

            if (y >= 127 && y < 175)
                currentPage = STORE;

            else if (y >= 175 && y < 223)
                currentPage = LIBRARY;

            else if (y >= 223 && y < 271)
                currentPage = COMMUNITY;

            else if (y >= 271 && y < 319)
                currentPage = PROFILE;

            else if (y >= 319 && y < 367)
                currentPage = INVENTORY;

            else if (y >= 367 && y < 415)
                currentPage = EQUIPMENT;

            else if (y >= 415 && y < 463)
                currentPage = SETTINGS;
        }

        if (currentPage == LIBRARY &&
            x >= 300 && x <= 480 &&
            y >= 365 && y <= 415) {

            statusText = L"Запуск CS2... (демонстрация)";
        }

        InvalidateRect(hwnd, NULL, TRUE);

        return 0;
    }

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcW(hwnd, msg, wParam, lParam);
}

int WINAPI wWinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    PWSTR,
    int nCmdShow
) {

    fontMain = CreateFontW(
        18, 0, 0, 0, FW_NORMAL,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Arial"
    );

    fontTitle = CreateFontW(
        28, 0, 0, 0, FW_BOLD,
        FALSE, FALSE, FALSE,
        DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,
        CLEARTYPE_QUALITY,
        DEFAULT_PITCH,
        L"Arial"
    );

    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"FakeSteamWindow";
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.hbrBackground = CreateSolidBrush(BG);

    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        L"FakeSteamWindow",
        L"Steam - Counter-Strike 2",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        1200,
        760,
        NULL,
        NULL,
        hInstance,
        NULL
    );

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;

    while (GetMessageW(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    DeleteObject(fontMain);
    DeleteObject(fontTitle);

    return 0;
}
