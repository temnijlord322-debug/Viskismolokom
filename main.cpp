#include <windows.h>
#include <string>
#include <vector>

#pragma comment(lib, "user32.lib")

constexpr int WINDOW_COUNT = 100000;
constexpr int TEXT_COUNT = 100000;

const wchar_t CLASS_NAME[] = L"HelloWorldWindow";

std::wstring helloText;
std::vector<HWND> windowsList;

// Создаём текст из 100 надписей
void CreateHelloText()
{
    helloText.clear();

    for (int i = 0; i < TEXT_COUNT; ++i)
    {
        helloText += L"Hello World!\r\n";
    }
}

// Обработка окна
LRESULT CALLBACK WindowProc(
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

            RECT rect;
            GetClientRect(hwnd, &rect);

            HFONT font = CreateFontW(
                18,
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

            HFONT oldFont =
                (HFONT)SelectObject(hdc, font);

            DrawTextW(
                hdc,
                helloText.c_str(),
                -1,
                &rect,
                DT_LEFT | DT_TOP
            );

            SelectObject(hdc, oldFont);
            DeleteObject(font);

            EndPaint(hwnd, &ps);

            return 0;
        }

        case WM_CLOSE:
        {
            DestroyWindow(hwnd);
            return 0;
        }

        case WM_DESTROY:
        {
            for (auto it = windowsList.begin();
                 it != windowsList.end();
                 ++it)
            {
                if (*it == hwnd)
                {
                    windowsList.erase(it);
                    break;
                }
            }

            if (windowsList.empty())
            {
                PostQuitMessage(0);
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

int WINAPI WinMain(
    HINSTANCE hInstance,
    HINSTANCE,
    LPSTR,
    int)
{
    CreateHelloText();

    // Регистрируем класс окна
    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;

    // Курсор без LoadCursorW/LoadCursorA-проблемы
    wc.hCursor = LoadCursor(
        nullptr,
        IDC_ARROW
    );

    wc.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc))
    {
        MessageBoxW(
            nullptr,
            L"Не удалось зарегистрировать класс окна.",
            L"Ошибка",
            MB_OK | MB_ICONERROR
        );

        return 1;
    }

    // Создаём 100 настоящих окон
    for (int i = 0; i < WINDOW_COUNT; ++i)
    {
        int column = i % 10;
        int row = i / 10;

        int x = 30 + column * 70;
        int y = 30 + row * 70;

        HWND hwnd = CreateWindowExW(
            0,
            CLASS_NAME,
            L"Hello World!",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            x,
            y,
            450,
            350,
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );

        if (hwnd != nullptr)
        {
            windowsList.push_back(hwnd);
        }
    }

    // Главный цикл
    MSG msg = {};

    while (GetMessageW(
        &msg,
        nullptr,
        0,
        0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }

    return 0;
}
