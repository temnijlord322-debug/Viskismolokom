#include <windows.h>
#include <string>

#pragma comment(lib, "user32.lib")

#define ID_EDIT 1001
#define ID_OK   1002

const char MAIN_CLASS[]  = "DemoMainWindow";
const char HELLO_CLASS[] = "DemoHelloWindow";

HWND g_mainWindow = nullptr;

// ============================================================
// Закрываем только окна этой программы
// ============================================================

BOOL CALLBACK CloseOwnWindows(HWND hwnd, LPARAM)
{
    DWORD pid = 0;

    GetWindowThreadProcessId(hwnd, &pid);

    if (pid == GetCurrentProcessId())
        DestroyWindow(hwnd);

    return TRUE;
}

// ============================================================
// Окна Hello World!
// ============================================================

LRESULT CALLBACK HelloWindowProc(
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

        // Зелёный текст
        SetTextColor(
            hdc,
            RGB(0, 255, 0)
        );

        // Несколько надписей
        for (int i = 0; i < 10; ++i)
        {
            RECT textRect = rc;

            textRect.top =
                10 + i * 22;

            textRect.bottom =
                textRect.top + 22;

            DrawTextA(
                hdc,
                "Hello World!",
                -1,
                &textRect,
                DT_CENTER |
                DT_SINGLELINE
            );
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

    return DefWindowProcA(
        hwnd,
        msg,
        wParam,
        lParam
    );
}

// ============================================================
// Главное окно
// ============================================================

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
        int screenWidth =
            GetSystemMetrics(SM_CXSCREEN);

        // ----------------------------------------------------
        // Заголовок
        // ----------------------------------------------------

        HWND title = CreateWindowA(
            "STATIC",
            "ДЕМОНСТРАЦИОННОЕ ОКНО",
            WS_VISIBLE |
            WS_CHILD |
            SS_CENTER,
            0,
            160,
            screenWidth,
            40,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );

        // ----------------------------------------------------
        // Инструкция
        // ----------------------------------------------------

        HWND instruction = CreateWindowA(
            "STATIC",
            "Введите пароль:",
            WS_VISIBLE |
            WS_CHILD |
            SS_CENTER,
            0,
            220,
            screenWidth,
            40,
            hwnd,
            nullptr,
            nullptr,
            nullptr
        );

        // ----------------------------------------------------
        // Поле пароля
        // ----------------------------------------------------

        HWND edit = CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "EDIT",
            "",
            WS_VISIBLE |
            WS_CHILD |
            ES_PASSWORD |
            ES_CENTER,
            screenWidth / 2 - 150,
            280,
            300,
            35,
            hwnd,
            (HMENU)ID_EDIT,
            nullptr,
            nullptr
        );

        // ----------------------------------------------------
        // Кнопка
        // ----------------------------------------------------

        HWND button = CreateWindowA(
            "BUTTON",
            "РАЗБЛОКИРОВАТЬ",
            WS_VISIBLE |
            WS_CHILD |
            BS_DEFPUSHBUTTON,
            screenWidth / 2 - 110,
            340,
            220,
            45,
            hwnd,
            (HMENU)ID_OK,
            nullptr,
            nullptr
        );

        // Убираем предупреждения компилятора
        (void)title;
        (void)instruction;
        (void)button;

        if (edit)
            SetFocus(edit);

        return 0;
    }

    // ========================================================
    // Нажата кнопка
    // ========================================================

    case WM_COMMAND:
    {
        if (LOWORD(wParam) == ID_OK)
        {
            HWND edit =
                GetDlgItem(
                    hwnd,
                    ID_EDIT
                );

            char password[64] = {};

            if (edit)
            {
                GetWindowTextA(
                    edit,
                    password,
                    sizeof(password)
                );
            }

            // ------------------------------------------------
            // Пароль 3252
            // ------------------------------------------------

            if (std::string(password) == "3252")
            {
                // Закрываем все окна,
                // принадлежащие только этой программе.
                EnumWindows(
                    CloseOwnWindows,
                    0
                );

                PostQuitMessage(0);
            }
            else
            {
                if (edit)
                {
                    SetWindowTextA(
                        edit,
                        ""
                    );
                }

                MessageBoxA(
                    hwnd,
                    "Неверный пароль!",
                    "Ошибка",
                    MB_OK |
                    MB_ICONERROR
                );
            }

            return 0;
        }

        break;
    }

    // ========================================================
    // Обычное закрытие
    // ========================================================

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(
        hwnd,
        msg,
        wParam,
        lParam
    );
}

// ============================================================
// WinMain
// ============================================================

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int)
{
    int screenWidth =
        GetSystemMetrics(SM_CXSCREEN);

    int screenHeight =
        GetSystemMetrics(SM_CYSCREEN);

    // ========================================================
    // Регистрация главного окна
    // ========================================================

    WNDCLASSA mainClass = {};

    mainClass.lpfnWndProc =
        MainWindowProc;

    mainClass.hInstance =
        hInstance;

    mainClass.lpszClassName =
        MAIN_CLASS;

    mainClass.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);

    mainClass.hCursor =
        LoadCursorA(
            nullptr,
            IDC_ARROW
        );

    if (!RegisterClassA(&mainClass))
    {
        DWORD error = GetLastError();

        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            char text[256];

            wsprintfA(
                text,
                "Ошибка регистрации главного окна.\n\nКод: %lu",
                error
            );

            MessageBoxA(
                nullptr,
                text,
                "Ошибка",
                MB_OK | MB_ICONERROR
            );

            return 0;
        }
    }

    // ========================================================
    // Регистрация Hello World
    // ========================================================

    WNDCLASSA helloClass = {};

    helloClass.lpfnWndProc =
        HelloWindowProc;

    helloClass.hInstance =
        hInstance;

    helloClass.lpszClassName =
        HELLO_CLASS;

    helloClass.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);

    helloClass.hCursor =
        LoadCursorA(
            nullptr,
            IDC_ARROW
        );

    if (!RegisterClassA(&helloClass))
    {
        DWORD error = GetLastError();

        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            char text[256];

            wsprintfA(
                text,
                "Ошибка регистрации Hello World.\n\nКод: %lu",
                error
            );

            MessageBoxA(
                nullptr,
                text,
                "Ошибка",
                MB_OK | MB_ICONERROR
            );

            return 0;
        }
    }

    // ========================================================
    // Создаём основное полноэкранное окно
    // ========================================================

    g_mainWindow = CreateWindowExA(
        0,
        MAIN_CLASS,
        "Password Demo",
        WS_POPUP |
        WS_VISIBLE,
        0,
        0,
        screenWidth,
        screenHeight,
        nullptr,
        nullptr,
        hInstance,
        nullptr
    );

    if (!g_mainWindow)
    {
        DWORD error = GetLastError();

        char text[256];

        wsprintfA(
            text,
            "Не удалось создать главное окно.\n\nКод Windows: %lu",
            error
        );

        MessageBoxA(
            nullptr,
            text,
            "Ошибка",
            MB_OK | MB_ICONERROR
        );

        return 0;
    }

    // ========================================================
    // Создаём 40 окон Hello World поверх основного окна
    // ========================================================

    const int HELLO_COUNT = 40;

    for (int i = 0; i < HELLO_COUNT; ++i)
    {
        const int width  = 280;
        const int height = 180;

        int maxX =
            screenWidth - width;

        int maxY =
            screenHeight - height;

        if (maxX < 1)
            maxX = 1;

        if (maxY < 1)
            maxY = 1;

        int x =
            (i * 67) % maxX;

        int y =
            (i * 43) % maxY;

        HWND hello = CreateWindowExA(
            WS_EX_TOPMOST |
            WS_EX_TOOLWINDOW |
            WS_EX_LAYERED,
            HELLO_CLASS,
            "",
            WS_POPUP |
            WS_VISIBLE,
            x,
            y,
            width,
            height,
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );

        if (hello)
        {
            // Полупрозрачность
            SetLayeredWindowAttributes(
                hello,
                0,
                190,
                LWA_ALPHA
            );

            SetWindowPos(
                hello,
                HWND_TOPMOST,
                x,
                y,
                width,
                height,
                SWP_SHOWWINDOW
            );
        }
    }

    // ========================================================
    // Цикл сообщений
    // ========================================================

    MSG msg = {};

    while (
        GetMessageA(
            &msg,
            nullptr,
            0,
            0) > 0)
    {
        TranslateMessage(&msg);

        DispatchMessageA(&msg);
    }

    return 0;
}
