#include <windows.h>
#include <string>
#include <vector>

#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")

#define ID_EDIT 1001
#define ID_OK   1002

const char MAIN_CLASS[]  = "DemoMainWindow";
const char HELLO_CLASS[] = "DemoHelloWindow";

HWND g_mainWindow = nullptr;

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
        SetTextColor(hdc, RGB(0, 255, 0));

        for (int i = 0; i < 10; ++i)
        {
            RECT textRect = rc;
            textRect.top = 10 + i * 22;
            textRect.bottom = textRect.top + 22;

            DrawTextA(
                hdc,
                "Hello World!",
                -1,
                &textRect,
                DT_CENTER | DT_SINGLELINE
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

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

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
        const int screenWidth = GetSystemMetrics(SM_CXSCREEN);

        CreateWindowA(
            "STATIC",
            "ДЕМОНСТРАЦИОННОЕ ОКНО",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 160, screenWidth, 40,
            hwnd, nullptr, nullptr, nullptr
        );

        CreateWindowA(
            "STATIC",
            "Введите пароль:",
            WS_VISIBLE | WS_CHILD | SS_CENTER,
            0, 220, screenWidth, 40,
            hwnd, nullptr, nullptr, nullptr
        );

        HWND edit = CreateWindowExA(
            WS_EX_CLIENTEDGE,
            "EDIT",
            "",
            WS_VISIBLE | WS_CHILD | ES_PASSWORD | ES_CENTER,
            screenWidth / 2 - 150, 280, 300, 35,
            hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_EDIT)),
            nullptr,
            nullptr
        );

        CreateWindowA(
            "BUTTON",
            "РАЗБЛОКИРОВАТЬ",
            WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
            screenWidth / 2 - 110, 340, 220, 45,
            hwnd,
            reinterpret_cast<HMENU>(static_cast<INT_PTR>(ID_OK)),
            nullptr,
            nullptr
        );

        if (edit)
            SetFocus(edit);

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

                MessageBoxA(
                    hwnd,
                    "Неверный пароль!",
                    "Ошибка",
                    MB_OK | MB_ICONERROR
                );
            }

            return 0;
        }
        break;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    }

    return DefWindowProcA(hwnd, msg, wParam, lParam);
}

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int)
{
    const int screenWidth  = GetSystemMetrics(SM_CXSCREEN);
    const int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    WNDCLASSA mainClass = {};
    mainClass.lpfnWndProc   = MainWindowProc;
    mainClass.hInstance     = hInstance;
    mainClass.lpszClassName = MAIN_CLASS;
    mainClass.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_WINDOW + 1));
    mainClass.hCursor       = LoadCursorA(nullptr, IDC_ARROW);

    if (!RegisterClassA(&mainClass))
    {
        const DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            char text[256];
            wsprintfA(text, "Ошибка регистрации главного окна.\n\nКод: %lu", error);
            MessageBoxA(nullptr, text, "Ошибка", MB_OK | MB_ICONERROR);
            return 0;
        }
    }

    WNDCLASSA helloClass = {};
    helloClass.lpfnWndProc   = HelloWindowProc;
    helloClass.hInstance     = hInstance;
    helloClass.lpszClassName = HELLO_CLASS;
    helloClass.hbrBackground = reinterpret_cast<HBRUSH>(static_cast<INT_PTR>(COLOR_WINDOW + 1));
    helloClass.hCursor       = LoadCursorA(nullptr, IDC_ARROW);

    if (!RegisterClassA(&helloClass))
    {
        const DWORD error = GetLastError();
        if (error != ERROR_CLASS_ALREADY_EXISTS)
        {
            char text[256];
            wsprintfA(text, "Ошибка регистрации Hello World.\n\nКод: %lu", error);
            MessageBoxA(nullptr, text, "Ошибка", MB_OK | MB_ICONERROR);
            return 0;
        }
    }

    g_mainWindow = CreateWindowExA(
        0,
        MAIN_CLASS,
        "Password Demo",
        WS_POPUP | WS_VISIBLE,
        0, 0, screenWidth, screenHeight,
        nullptr, nullptr, hInstance, nullptr
    );

    if (!g_mainWindow)
    {
        const DWORD error = GetLastError();
        char text[256];
        wsprintfA(text, "Не удалось создать главное окно.\n\nКод Windows: %lu", error);
        MessageBoxA(nullptr, text, "Ошибка", MB_OK | MB_ICONERROR);
        return 0;
    }

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
            HELLO_CLASS,
            "",
            WS_POPUP | WS_VISIBLE,
            x, y, width, height,
            nullptr, nullptr, hInstance, nullptr
        );

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
