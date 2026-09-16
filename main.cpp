#include <windows.h>
#include <string>
#include <vector>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

// ---------------- НАСТРОЙКИ ----------------

const wchar_t MAIN_CLASS[]   = L"PasswordMainWindow";
const wchar_t SYMBOL_CLASS[] = L"SymbolWindow";

const wchar_t PASSWORD[] = L"3252";

constexpr int LOAD_TIME_MS = 5000;
constexpr int LIMIT_TIME_MS = 60000;

// ИЗМЕНЕНО: 10000 окон
constexpr int SYMBOL_WINDOW_COUNT = 10000; 
const wchar_t SYMBOL[] = L"𰻞";

// ID элементов главного окна
constexpr int ID_PASSWORD = 1001;
constexpr int ID_BUTTON   = 1002;
constexpr int ID_TIMER    = 1003;

// --------------------------------------------

HWND g_mainWindow = nullptr;
HWND g_passwordEdit = nullptr;
HWND g_button = nullptr;

std::vector<HWND> g_symbolWindows;

int g_attempts = 0;
bool g_triggered = false;

// --------------------------------------------
// Текст для окон с символами
// --------------------------------------------

std::wstring MakeSymbolText()
{
    std::wstring text;

    // 1000 символов в каждом окне.
    for (int i = 0; i < 1000; ++i)
    {
        text += SYMBOL;
        text += L" ";

        if ((i + 1) % 20 == 0)
            text += L"\r\n";
    }

    return text;
}

std::wstring g_symbolText;

// --------------------------------------------
// Окно с символами
// --------------------------------------------

LRESULT CALLBACK SymbolWindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
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

        HFONT font = CreateFontW(
            22,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE,
            L"Segoe UI"
        );

        if (font)
        {
            HFONT oldFont =
                (HFONT)SelectObject(hdc, font);

            DrawTextW(
                hdc,
                g_symbolText.c_str(),
                -1,
                &rc,
                DT_LEFT | DT_TOP | DT_WORDBREAK
            );

            SelectObject(hdc, oldFont);
            DeleteObject(font);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
    {
        for (auto it = g_symbolWindows.begin();
             it != g_symbolWindows.end();
             ++it)
        {
            if (*it == hwnd)
            {
                g_symbolWindows.erase(it);
                break;
            }
        }

        return 0;
    }
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam
    );
}

// --------------------------------------------
// Открыть 10000 окон
// --------------------------------------------

void OpenSymbolWindows(HINSTANCE hInstance)
{
    if (g_triggered)
        return;

    g_triggered = true;

    KillTimer(g_mainWindow, ID_TIMER);

    g_symbolText = MakeSymbolText();

    // Сетка 100x100 для 10000 окон
    for (int i = 0; i < SYMBOL_WINDOW_COUNT; ++i)
    {
        // ИЗМЕНЕНО: делим на 100, чтобы получить сетку 100 на 100
        int column = i % 100;
        int row = i / 100;

        // ИЗМЕНЕНО: уменьшил смещение, чтобы окна хоть как-то помещались на экране
        int x = 10 + column * 15;
        int y = 10 + row * 15;

        HWND hwnd = CreateWindowExW(
            0,
            SYMBOL_CLASS,
            L"𰻞",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            x,
            y,
            150, // Уменьшил размер окна, чтобы влезло больше
            100,
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );

        if (hwnd)
            g_symbolWindows.push_back(hwnd);
    }

    // Главное окно больше не нужно.
    if (g_mainWindow)
    {
        DestroyWindow(g_mainWindow);
        g_mainWindow = nullptr;
    }
}

// --------------------------------------------
// Проверка пароля
// --------------------------------------------

void CheckPassword()
{
    wchar_t buffer[100] = {};

    GetWindowTextW(
        g_passwordEdit,
        buffer,
        100
    );

    if (wcscmp(buffer, PASSWORD) == 0)
    {
        // Правильный пароль —
        // закрываем только наше приложение.
        KillTimer(g_mainWindow, ID_TIMER);

        DestroyWindow(g_mainWindow);
        g_mainWindow = nullptr;

        return;
    }

    g_attempts++;

    SetWindowTextW(
        g_passwordEdit,
        L""
    );

    wchar_t message[100];

    if (g_attempts >= 6)
    {
        HINSTANCE hInstance =
            (HINSTANCE)GetWindowLongPtrW(
                g_mainWindow,
                GWLP_HINSTANCE
            );

        OpenSymbolWindows(hInstance);
        return;
    }

    wsprintfW(
        message,
        L"Неверный код.\nПопытка %d из 6.",
        g_attempts
    );

    MessageBoxW(
        g_mainWindow,
        message,
        L"Ошибка",
        MB_OK | MB_ICONWARNING
    );
}

// --------------------------------------------
// Главное окно
// --------------------------------------------

LRESULT CALLBACK MainWindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_CREATE:
    {
        CreateWindowW(
            L"STATIC",
            L"Введите код:",
            WS_VISIBLE | WS_CHILD,
            30,
            30,
            300,
            25,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );

        g_passwordEdit = CreateWindowExW(
            WS_EX_CLIENTEDGE,
            L"EDIT",
            L"",
            WS_VISIBLE |
            WS_CHILD |
            ES_PASSWORD |
            ES_CENTER,
            30,
            65,
            300,
            35,
            hwnd,
            (HMENU)ID_PASSWORD,
            nullptr,
            nullptr
        );

        g_button = CreateWindowW(
            L"BUTTON",
            L"Проверить",
            WS_VISIBLE |
            WS_CHILD |
            BS_PUSHBUTTON,
            30,
            115,
            300,
            40,
            hwnd,
            (HMENU)ID_BUTTON,
            nullptr,
            nullptr
        );

        CreateWindowW(
            L"STATIC",
            L"Осталось: 60 секунд",
            WS_VISIBLE | WS_CHILD,
            30,
            170,
            300,
            25,
            hwnd,
            (HMENU)ID_TIMER,
            nullptr,
            nullptr
        );

        SetTimer(
            hwnd,
            ID_TIMER,
            1000,
            nullptr
        );

        return 0;
    }

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_BUTTON)
        {
            CheckPassword();
        }

        if (LOWORD(wParam) == ID_PASSWORD &&
            HIWORD(wParam) == EN_UPDATE)
        {
            // Ничего дополнительно не делаем.
        }

        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == ID_TIMER)
        {
            static int seconds = 60;

            seconds--;

            wchar_t text[100];

            wsprintfW(
                text,
                L"Осталось: %d секунд",
                seconds
            );

            HWND label = GetDlgItem(
                hwnd,
                ID_TIMER
            );

            if (label)
                SetWindowTextW(label, text);

            if (seconds <= 0)
            {
                HINSTANCE hInstance =
                    (HINSTANCE)GetWindowLongPtrW(
                        hwnd,
                        GWLP_HINSTANCE
                    );

                OpenSymbolWindows(hInstance);
            }
        }

        return 0;
    }

    case WM_CLOSE:
    {
        // Не закрываем главное окно обычным крестиком.
        MessageBoxW(
            hwnd,
            L"Сначала введите правильный код.",
            L"Приложение",
            MB_OK | MB_ICONINFORMATION
        );

        return 0;
    }
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam
    );
}

// --------------------------------------------
// Окно загрузки
// --------------------------------------------

LRESULT CALLBACK LoadingWindowProc(
    HWND hwnd,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    switch (msg)
    {
    case WM_PAINT:
    {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);

        RECT rc;
        GetClientRect(hwnd, &rc);

        FillRect(
            hdc,
            &rc,
            (HBRUSH)GetStockObject(BLACK_BRUSH)
        );

        SetTextColor(
            hdc,
            RGB(255, 255, 255)
        );

        SetBkMode(
            hdc,
            TRANSPARENT
        );

        HFONT font = CreateFontW(
            30,
            0,
            0,
            0,
            FW_NORMAL,
            FALSE,
            FALSE,
            FALSE,
            DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS,
            CLIP_DEFAULT_PRECIS,
            DEFAULT_QUALITY,
            DEFAULT_PITCH,
            L"Segoe UI"
        );

        if (font)
        {
            HFONT oldFont =
                (HFONT)SelectObject(hdc, font);

            DrawTextW(
                hdc,
                L"Загрузка...",
                -1,
                &rc,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE
            );

            SelectObject(hdc, oldFont);
            DeleteObject(font);
        }

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_TIMER:
    {
        if (wParam == 1)
        {
            KillTimer(hwnd, 1);
            DestroyWindow(hwnd);
        }

        return 0;
    }

    case WM_CLOSE:
        return 0;
    }

    return DefWindowProcW(
        hwnd,
        msg,
        wParam,
        lParam
    );
}

// --------------------------------------------
// WinMain
// --------------------------------------------

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int nCmdShow)
{
    // -------------------------------
    // Класс загрузки
    // -------------------------------

    WNDCLASSW loadingClass = {};

    loadingClass.lpfnWndProc =
        LoadingWindowProc;

    loadingClass.hInstance =
        hInstance;

    loadingClass.lpszClassName =
        L"LoadingWindow";

    loadingClass.hCursor =
        LoadCursor(nullptr, IDC_ARROW);

    loadingClass.hbrBackground =
        (HBRUSH)GetStockObject(BLACK_BRUSH);

    RegisterClassW(&loadingClass);

    // -------------------------------
    // Показываем загрузку
    // -------------------------------

    HWND loading = CreateWindowExW(
        WS_EX_TOPMOST,
        L"LoadingWindow",
        L"Загрузка",
        WS_POPUP | WS_VISIBLE,
        0,
        0,
        GetSystemMetrics(SM_CXSCREEN),
        GetSystemMetrics(SM_CYSCREEN),
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    SetTimer(
        loading,
        1,
        LOAD_TIME_MS,
        nullptr
    );

    // Ждём завершения загрузки
    MSG msg;

    while (GetMessageW(
        &msg,
        nullptr,
        0,
        0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);

        if (!IsWindow(loading))
            break;
    }

    // -------------------------------
    // Класс главного окна
    // -------------------------------

    WNDCLASSW mainClass = {};

    mainClass.lpfnWndProc =
        MainWindowProc;

    mainClass.hInstance =
        hInstance;

    mainClass.lpszClassName =
        MAIN_CLASS;

    mainClass.hCursor =
        LoadCursor(nullptr, IDC_ARROW);

    mainClass.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&mainClass))
        return 1;

    // -------------------------------
    // Класс окон символов
    // -------------------------------

    WNDCLASSW symbolClass = {};

    symbolClass.lpfnWndProc =
        SymbolWindowProc;

    symbolClass.hInstance =
        hInstance;

    symbolClass.lpszClassName =
        SYMBOL_CLASS;

    symbolClass.hCursor =
        LoadCursor(nullptr, IDC_ARROW);

    symbolClass.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&symbolClass))
        return 1;

    // -------------------------------
    // Главное окно
    // -------------------------------

    g_mainWindow = CreateWindowExW(
        0,
        MAIN_CLASS,
        L"Моё приложение",
        WS_OVERLAPPEDWINDOW |
        WS_VISIBLE,
        400,
        200,
        380,
        280,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_mainWindow)
        return 1;

    ShowWindow(
        g_mainWindow,
        nCmdShow
    );

    UpdateWindow(g_mainWindow);

    // -------------------------------
    // Основной цикл
    // -------------------------------

    while (GetMessageW(
        &msg,
        nullptr,
        0,
        0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    // Закрываем оставшиеся окна
    for (HWND hwnd : g_symbolWindows)
    {
        if (IsWindow(hwnd))
            DestroyWindow(hwnd);
    }

    return 0;
}
