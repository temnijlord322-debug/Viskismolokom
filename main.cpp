#include <windows.h>
#include <string>
#include <vector>

#pragma comment(lib, "user32.lib")

const wchar_t CLASS_NAME[] = L"HelloWorld100x100";

const int WINDOW_COUNT = 10000;
const int TEXT_COUNT = 10000;

std::wstring g_text;
std::vector<HWND> g_windows;

std::wstring MakeText()
{
    std::wstring text;

    for (int i = 0; i < TEXT_COUNT; ++i)
    {
        text += L"Hello World!\r\n";
    }

    return text;
}

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

        RECT rc;
        GetClientRect(hwnd, &rc);

        HFONT font = CreateFontW(
            20, 0, 0, 0,
            FW_NORMAL,
            FALSE, FALSE, FALSE,
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
            g_text.c_str(),
            -1,
            &rc,
            DT_LEFT | DT_TOP
        );

        SelectObject(hdc, oldFont);
        DeleteObject(font);

        EndPaint(hwnd, &ps);
        return 0;
    }

    case WM_CLOSE:
        DestroyWindow(hwnd);
        return 0;

    case WM_DESTROY:
    {
        for (auto it = g_windows.begin();
             it != g_windows.end(); ++it)
        {
            if (*it == hwnd)
            {
                g_windows.erase(it);
                break;
            }
        }

        if (g_windows.empty())
            PostQuitMessage(0);

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
    // Создаём 100 надписей один раз
    g_text = MakeText();

    WNDCLASSW wc = {};

    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hCursor = LoadCursorW(
        nullptr,
        IDC_ARROW
    );
    wc.hbrBackground =
        (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&wc))
        return 1;

    // Создаём 100 настоящих окон
    for (int i = 0; i < WINDOW_COUNT; ++i)
    {
        int column = i % 10;
        int row = i / 10;

        int x = 50 + column * 60;
        int y = 50 + row * 60;

        HWND hwnd = CreateWindowExW(
            0,
            CLASS_NAME,
            L"Hello World!",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE,
            x,
            y,
            400,
            300,
            nullptr,
            nullptr,
            hInstance,
            nullptr
        );

        if (hwnd)
            g_windows.push_back(hwnd);
    }

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
